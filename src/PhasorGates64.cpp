#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DSP/HCVTiming.h"

struct PhasorGates64 : HCVModule
{
    static constexpr int NUM_STEPS = 64;
    static constexpr float STEPS_CV_SCALE = (NUM_STEPS - 1)/5.0f;

	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        WIDTH_PARAM,
        WIDTHCV_PARAM,
        DETECTION_PARAM,

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
        GATES_OUTPUT,
        TRIGS_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ENUMS(GATE_LIGHTS, NUM_STEPS*3),
        NUM_LIGHTS
	};

    bool gates[NUM_STEPS] = {};
    bool smartDetection = true;
    dsp::BooleanTrigger gateTriggers[NUM_STEPS];

    HCVPhasorSlopeDetector slopeDetectors[16];
    HCVPhasorStepDetector stepDetectors[16];
    HCVTriggeredGate triggers[16];

	PhasorGates64()
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
        configInput(PHASOR_INPUT, "Phasor CV");

        configOutput(GATES_OUTPUT, "Gate Sequence");
        configOutput(TRIGS_OUTPUT, "Trigger Sequence");

        for (int i = 0; i < NUM_STEPS; i++) 
        {
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


void PhasorGates64::process(const ProcessArgs &args)
{
    const float stepsKnob = params[STEPS_PARAM].getValue();
    const float stepsDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float widthKnob = params[WIDTH_PARAM].getValue();
    const float widthDepth = params[WIDTHCV_PARAM].getValue();

    int numChannels = setupPolyphonyForAllOutputs();
    int lightIndex = 0;

    smartDetection = params[DETECTION_PARAM].getValue() > 0.0f;

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

        if(smartDetection)
        {
            bool reversePhasor = slopeDetectors[i](normalizedPhasor) < 0.0f;
            bool isZero = normalizedPhasor == 0.0f;

            if(slopeDetectors[i].isPhasorAdvancing() || !isZero)
            {
                float gate;
                if (reversePhasor) gate = (1.0f - fractionalIndex) < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;
                else gate = fractionalIndex < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;

                outputs[GATES_OUTPUT].setVoltage(gates[currentIndex] ? gate : 0.0f, i);

                bool trigger = gate && gates[currentIndex];
                outputs[TRIGS_OUTPUT].setVoltage(triggers[i].process(trigger) ? HCV_PHZ_GATESCALE : 0.0f, i);
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
        }

        

        if(i == 0) lightIndex = currentIndex;
    }

    bool isPlaying = slopeDetectors[0].isPhasorAdvancing();

    // Gate buttons
    for (int i = 0; i < NUM_STEPS; i++) 
    {
        if (gateTriggers[i].process(params[GATE_PARAMS + i].getValue())) {
            gates[i] ^= true;
        }
        lights[GATE_LIGHTS + 3 * i + 0].setBrightness(i >= stepsKnob); //red
        lights[GATE_LIGHTS + 3 * i + 1].setBrightness(gates[i]); //green
        lights[GATE_LIGHTS + 3 * i + 2].setSmoothBrightness(isPlaying && lightIndex == i, args.sampleTime); //blue
    }
}

struct PhasorGates64Widget : HCVModuleWidget { PhasorGates64Widget(PhasorGates64 *module); };

PhasorGates64Widget::PhasorGates64Widget(PhasorGates64 *module)
{
    setSkinPath("res/PhasorGates64.svg");
    initializeWidget(module);
    
    //////PARAMS//////
    int knobY = 60;
    createParamComboVertical(15, knobY, PhasorGates64::WIDTH_PARAM, PhasorGates64::WIDTHCV_PARAM, PhasorGates64::WIDTHCV_INPUT);
    createParamComboVertical(70, knobY, PhasorGates64::STEPS_PARAM, PhasorGates64::STEPSCV_PARAM, PhasorGates64::STEPSCV_INPUT);
    
    createHCVSwitchVert(89, 255, PhasorGates64::DETECTION_PARAM);

    //////INPUTS//////
    int jackX = 20;
    createInputPort(jackX, 248, PhasorGates64::PHASOR_INPUT);

    //////OUTPUTS/////
    createOutputPort(jackX, 310, PhasorGates64::GATES_OUTPUT);
    createOutputPort(78, 310, PhasorGates64::TRIGS_OUTPUT);

    for (int i = 0; i < PhasorGates64::NUM_STEPS; i++)
    {
        int buttonY = 55 + (i/8)*40;
        int buttonX = 130 + 30 * (i%8);
        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(buttonX, buttonY), 
        module, PhasorGates64::GATE_PARAMS + i, PhasorGates64::GATE_LIGHTS + 3 * i));
    }
    
    
}

Model *modelPhasorGates64 = createModel<PhasorGates64, PhasorGates64Widget>("PhasorGates64");
