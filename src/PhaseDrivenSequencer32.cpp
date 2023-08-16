#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DSP/HCVTiming.h"

struct PhaseDrivenSequencer32 : HCVModule
{
    static constexpr int NUM_STEPS = 32;
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
        RUN_INPUT,
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
        STEPS_LIGHT,
        SLEW_LIGHT,
        SH_LIGHT,
        GATES_LIGHT,
        RUN_LIGHT,
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

	PhaseDrivenSequencer32()
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


void PhaseDrivenSequencer32::process(const ProcessArgs &args)
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

        bool active = true;
        if(inputs[RUN_INPUT].isConnected())
        {
            active = inputs[RUN_INPUT].getPolyVoltage(i) >= 1.0f;
        }

        const float phasorIn = active ? inputs[PHASOR_INPUT].getPolyVoltage(i) : 0.0f;
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
            outputs[GATES_OUTPUT].setVoltage(gates[currentIndex] && active ? gate : 0.0f, i);

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

    bool active = true;
    if(inputs[RUN_INPUT].isConnected())
    {
        active = inputs[RUN_INPUT].getPolyVoltage(0) >= 1.0f;
    }
    bool isPlaying = slopeDetectors[0].isPhasorAdvancing() && active;

    // Gate lights
    for (int i = 0; i < NUM_STEPS; i++) 
    {
        lights[GATE_LIGHTS + 3 * i + 0].setBrightness(i >= stepsKnob); //red
        lights[GATE_LIGHTS + 3 * i + 1].setBrightness(gates[i]); //green
        lights[GATE_LIGHTS + 3 * i + 2].setSmoothBrightness(isPlaying && lightIndex == i, args.sampleTime); //blue
    }

    lights[RUN_LIGHT].setBrightness(active ? 1.0f : 0.0f);
    setLightFromOutput(STEPS_LIGHT, STEPS_OUTPUT, 0.2f);
    setLightFromOutput(SLEW_LIGHT, SLEW_OUTPUT, 0.2f);
    setLightFromOutput(SH_LIGHT, SH_OUTPUT, 0.2f);
    setLightFromOutput(GATES_LIGHT, GATES_OUTPUT);
}

struct PhaseDrivenSequencer32Widget : HCVModuleWidget { PhaseDrivenSequencer32Widget(PhaseDrivenSequencer32 *module); };

PhaseDrivenSequencer32Widget::PhaseDrivenSequencer32Widget(PhaseDrivenSequencer32 *module)
{
    setSkinPath("res/PhaseDrivenSequencer32.svg");
    initializeWidget(module);
    
    //////PARAMS//////
    int knobY = 60;
    createParamComboVertical(15, knobY, PhaseDrivenSequencer32::WIDTH_PARAM, PhaseDrivenSequencer32::WIDTHCV_PARAM, PhaseDrivenSequencer32::WIDTHCV_INPUT);
    createParamComboVertical(70, knobY, PhaseDrivenSequencer32::STEPS_PARAM, PhaseDrivenSequencer32::STEPSCV_PARAM, PhaseDrivenSequencer32::STEPSCV_INPUT);
    
    createHCVSwitchVert(53, 208, PhaseDrivenSequencer32::DETECTION_PARAM);

    //////INPUTS//////
    int jackX = 20;
    int jackX2 = 78;
    int inputY = 248;
    createInputPort(jackX,  inputY, PhaseDrivenSequencer32::PHASOR_INPUT);
    createInputPort(jackX2, inputY, PhaseDrivenSequencer32::RUN_INPUT);

    //////OUTPUTS/////
    int outJackX = 78;
    int spacing = 45;
    int outY = 310;
    createOutputPort(outJackX, outY, PhaseDrivenSequencer32::STEPS_OUTPUT);
    createOutputPort(outJackX + spacing, outY, PhaseDrivenSequencer32::SLEW_OUTPUT);
    createOutputPort(outJackX + spacing*2, outY, PhaseDrivenSequencer32::SH_OUTPUT);
    createOutputPort(outJackX + spacing*3, outY, PhaseDrivenSequencer32::GATES_OUTPUT);
    createOutputPort(outJackX + spacing*4, outY, PhaseDrivenSequencer32::TRIGS_OUTPUT);

    for (int i = 0; i < PhaseDrivenSequencer32::NUM_STEPS; i++)
    {
        int buttonY = 75 + (i/8)*66;
        int buttonX = 130 + 30 * (i%8);

        int sliderY = buttonY - 25.0f;
        int sliderX = buttonX;

        createHCVTrimpotCentered(sliderX, sliderY, PhaseDrivenSequencer32::VOLT_PARAMS + i);

        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(buttonX, buttonY), 
        module, PhaseDrivenSequencer32::GATE_PARAMS + i, PhaseDrivenSequencer32::GATE_LIGHTS + 3 * i));
    }

    createHCVGreenLightForJack(jackX2, inputY, PhaseDrivenSequencer32::RUN_LIGHT);
    
    createHCVRedLightForJack(outJackX, outY, PhaseDrivenSequencer32::STEPS_LIGHT);
    createHCVRedLightForJack(outJackX + spacing, outY, PhaseDrivenSequencer32::SLEW_LIGHT);
    createHCVRedLightForJack(outJackX + spacing*2, outY, PhaseDrivenSequencer32::SH_LIGHT);
    createHCVRedLightForJack(outJackX + spacing*3, outY, PhaseDrivenSequencer32::GATES_LIGHT);
}

Model *modelPhaseDrivenSequencer32 = createModel<PhaseDrivenSequencer32, PhaseDrivenSequencer32Widget>("PhaseDrivenSequencer32");
