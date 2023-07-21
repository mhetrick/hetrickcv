#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DSP/HCVTiming.h"

struct PhaseDrivenSequencer : HCVModule
{
    static constexpr int NUM_STEPS = 16;
    static constexpr float STEPS_CV_SCALE = (NUM_STEPS - 1)/5.0f;

	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        WIDTH_PARAM,
        WIDTHCV_PARAM,
        DETECTION_PARAM,

        ENUMS(VOLT_PARAMS, NUM_STEPS),
        ENUMS(GATE_PARAMS, NUM_STEPS),

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        WIDTHCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        STEPS_OUTPUT,
        SLEW_OUTPUT,
        SH_OUTPUT,
        GATES_OUTPUT,
        TRIGS_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ENUMS(GATE_LIGHTS, NUM_STEPS*3),
        NUM_LIGHTS
	};

    float volts[NUM_STEPS] = {};
    float heldVolts[NUM_STEPS] = {};

    bool gates[NUM_STEPS] = {};
    bool smartDetection = true;
    dsp::BooleanTrigger gateTriggers[NUM_STEPS];

    HCVPhasorSlopeDetector slopeDetectors[16];
    HCVPhasorStepDetector stepDetectors[16];
    HCVTriggeredGate triggers[16];

	PhaseDrivenSequencer()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(STEPS_PARAM, 1.0, NUM_STEPS, NUM_STEPS, "Steps");
        configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");

        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(WIDTH_PARAM, -5.0, 5.0, 0.0, "Gate Width");
        configParam(WIDTHCV_PARAM, -1.0, 1.0, 1.0, "Gate Width CV Depth");

        configSwitch(DETECTION_PARAM, 0.0, 1.0, 1.0, "Detection Mode", {"Raw", "Smart (Detect Playback and Reverse)"});

        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(WIDTHCV_INPUT, "Gate Width CV");
        configInput(PHASOR_INPUT, "Phasor");

        configOutput(GATES_OUTPUT, "Gate Sequence");
        configOutput(TRIGS_OUTPUT, "Trigger Sequence");

        for (int i = 0; i < NUM_STEPS; i++) 
        {
            configParam(VOLT_PARAMS + i, -5.0f, 5.0f, 0.0f, string::f("Step %d Voltage", i + 1), "V");
			configButton(GATE_PARAMS + i, string::f("Gate %d Toggle", i + 1));
		}

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {
        for (int i = 0; i < NUM_STEPS; i++) 
        {
			gates[i] = false;
            heldVolts[i] = 0.0f;
		}
    }

    void onRandomize() override 
    {
		for (int i = 0; i < NUM_STEPS; i++) 
        {
			gates[i] = random::get<bool>();
		}
	}

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
		// states
        json_t *gateStatesJ = json_array();
        for (int i = 0; i < NUM_STEPS; i++)
        {
			json_t *gateStateJ = json_boolean(gates[i]);
            json_array_append_new(gateStatesJ, gateStateJ);
		}
        json_object_set_new(rootJ, "gateStates", gateStatesJ);
		return rootJ;
	}
    void dataFromJson(json_t *rootJ) override
    {
		// states
        json_t *gateStatesJ = json_object_get(rootJ, "gateStates");
        if (gateStatesJ)
        {
            for (int i = 0; i < NUM_STEPS; i++)
            {
				json_t *stateJ = json_array_get(gateStatesJ, i);
				if (stateJ)
					gates[i] = json_boolean_value(stateJ);
			}
        }
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhaseDrivenSequencer::process(const ProcessArgs &args)
{
    const float stepsKnob = params[STEPS_PARAM].getValue();
    const float stepsDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float widthKnob = params[WIDTH_PARAM].getValue();
    const float widthDepth = params[WIDTHCV_PARAM].getValue();

    int numChannels = setupPolyphonyForAllOutputs();
    int lightIndex = 0;

    smartDetection = params[DETECTION_PARAM].getValue() > 0.0f;

    for (int i = 0; i < NUM_STEPS; i++) 
    {
        if (gateTriggers[i].process(params[GATE_PARAMS + i].getValue())) 
        {
            gates[i] ^= true;
        }

        volts[i] = params[VOLT_PARAMS + i].getValue();
    }

    for (int i = 0; i < numChannels; i++)
    {
        float numSteps = stepsKnob + (stepsDepth * inputs[STEPSCV_INPUT].getPolyVoltage(i));
        numSteps = clamp(numSteps, 1.0f, float(NUM_STEPS));

        float pulseWidth = widthKnob + (widthDepth * inputs[WIDTHCV_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;

        const float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        stepDetectors[i].setNumberSteps(numSteps);
        bool triggered = stepDetectors[i](normalizedPhasor);
        int currentIndex = stepDetectors[i].getCurrentStep();
        float fractionalIndex = stepDetectors[i].getFractionalStep();

        bool reversePhasor = slopeDetectors[i](normalizedPhasor) < 0.0f;

        float stepOutput = volts[currentIndex];

        if(smartDetection)
        {
            bool isZero = normalizedPhasor == 0.0f;

            if(slopeDetectors[i].isPhasorAdvancing() || !isZero)
            {
                float gate;
                if (reversePhasor) gate = (1.0f - fractionalIndex) < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;
                else gate = fractionalIndex < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;

                outputs[GATES_OUTPUT].setVoltage(gates[currentIndex] ? gate : 0.0f, i);

                bool trigger = gate && gates[currentIndex];
                outputs[TRIGS_OUTPUT].setVoltage(triggers[i].process(trigger) ? HCV_PHZ_GATESCALE : 0.0f, i);

                if(trigger) heldVolts[i] = stepOutput;
            }
            else
            {
                outputs[GATES_OUTPUT].setVoltage(0.0f, i);
                outputs[TRIGS_OUTPUT].setVoltage(0.0f, i);
            }
        }
        else
        {
            const float gate = fractionalIndex < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;
            outputs[GATES_OUTPUT].setVoltage(gates[currentIndex] ? gate : 0.0f, i);

            bool trigger = gate && gates[currentIndex];
            outputs[TRIGS_OUTPUT].setVoltage(triggers[i].process(trigger) ? HCV_PHZ_GATESCALE : 0.0f, i);

            if(trigger) heldVolts[i] = stepOutput;
        }

        int nextStep = currentIndex + 1;
        if(nextStep >= numSteps) nextStep = 0;
        float slewedOut = LERP(fractionalIndex, volts[nextStep], volts[currentIndex]);

        outputs[STEPS_OUTPUT].setVoltage(stepOutput, i);
        outputs[SLEW_OUTPUT].setVoltage(slewedOut, i);
        outputs[SH_OUTPUT].setVoltage(heldVolts[i], i);

        if(i == 0) lightIndex = currentIndex;
    }

    bool isPlaying = slopeDetectors[0].isPhasorAdvancing();

    // Gate lights
    for (int i = 0; i < NUM_STEPS; i++) 
    {
        lights[GATE_LIGHTS + 3 * i + 0].setBrightness(i >= stepsKnob); //red
        lights[GATE_LIGHTS + 3 * i + 1].setBrightness(gates[i]); //green
        lights[GATE_LIGHTS + 3 * i + 2].setSmoothBrightness(isPlaying && lightIndex == i, args.sampleTime); //blue
    }
}

struct PhaseDrivenSequencerWidget : HCVModuleWidget { PhaseDrivenSequencerWidget(PhaseDrivenSequencer *module); };

PhaseDrivenSequencerWidget::PhaseDrivenSequencerWidget(PhaseDrivenSequencer *module)
{
    setSkinPath("res/PhaseDrivenSequencer.svg");
    initializeWidget(module);
    
    //////PARAMS//////
    int knobY = 60;
    createParamComboVertical(15, knobY, PhaseDrivenSequencer::WIDTH_PARAM, PhaseDrivenSequencer::WIDTHCV_PARAM, PhaseDrivenSequencer::WIDTHCV_INPUT);
    createParamComboVertical(70, knobY, PhaseDrivenSequencer::STEPS_PARAM, PhaseDrivenSequencer::STEPSCV_PARAM, PhaseDrivenSequencer::STEPSCV_INPUT);
    
    createHCVSwitchVert(89, 255, PhaseDrivenSequencer::DETECTION_PARAM);

    //////INPUTS//////
    int jackX = 20;
    createInputPort(jackX, 248, PhaseDrivenSequencer::PHASOR_INPUT);

    //////OUTPUTS/////
    int spacing = 45;
    createOutputPort(jackX, 310, PhaseDrivenSequencer::STEPS_OUTPUT);
    createOutputPort(jackX + spacing, 310, PhaseDrivenSequencer::SLEW_OUTPUT);
    createOutputPort(jackX + spacing*2, 310, PhaseDrivenSequencer::SH_OUTPUT);
    createOutputPort(jackX + spacing*3, 310, PhaseDrivenSequencer::GATES_OUTPUT);
    createOutputPort(jackX + spacing*4, 310, PhaseDrivenSequencer::TRIGS_OUTPUT);

    for (int i = 0; i < PhaseDrivenSequencer::NUM_STEPS; i++)
    {
        int buttonY = 75 + (i/4)*66;
        int buttonX = 130 + 30 * (i%4);

        int sliderY = buttonY - 25.0f;
        int sliderX = buttonX;

        createHCVTrimpotCentered(sliderX, sliderY, PhaseDrivenSequencer::VOLT_PARAMS + i);

        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(buttonX, buttonY), 
        module, PhaseDrivenSequencer::GATE_PARAMS + i, PhaseDrivenSequencer::GATE_LIGHTS + 3 * i));
    }
    
    
}

Model *modelPhaseDrivenSequencer = createModel<PhaseDrivenSequencer, PhaseDrivenSequencerWidget>("PhaseDrivenSequencer");
