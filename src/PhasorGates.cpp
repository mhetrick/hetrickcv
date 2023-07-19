#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"

struct PhasorGates : HCVModule
{
    static constexpr int NUM_STEPS = 8;
    static constexpr float STEPS_CV_SCALE = (NUM_STEPS - 1)/5.0f;

	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        WIDTH_PARAM,
        WIDTHCV_PARAM,

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
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ENUMS(GATE_LIGHTS, NUM_STEPS*3),
        NUM_LIGHTS
	};

    bool gates[NUM_STEPS] = {};
    dsp::BooleanTrigger gateTriggers[NUM_STEPS];

    HCVPhasorSlopeDetector slopeDetectors[16];

	PhasorGates()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(STEPS_PARAM, 1.0, NUM_STEPS, NUM_STEPS, "Steps");
        configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");

        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(WIDTH_PARAM, -5.0, 5.0, 0.0, "Gate Width");
        configParam(WIDTHCV_PARAM, -1.0, 1.0, 1.0, "Gate Width CV Depth");

        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(WIDTHCV_INPUT, "Gate Width CV");
        configInput(PHASOR_INPUT, "Phasor CV");

        configOutput(GATES_OUTPUT, "Gate Sequence");

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


void PhasorGates::process(const ProcessArgs &args)
{
    const float stepsKnob = params[STEPS_PARAM].getValue();
    const float stepsDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float widthKnob = params[WIDTH_PARAM].getValue();
    const float widthDepth = params[WIDTHCV_PARAM].getValue();

    int numChannels = setupPolyphonyForAllOutputs();
    int lightIndex = 0;

    for (int i = 0; i < numChannels; i++)
    {
        float numSteps = stepsKnob + (stepsDepth * inputs[STEPSCV_INPUT].getPolyVoltage(i));
        numSteps = clamp(numSteps, 1.0f, float(NUM_STEPS));

        float pulseWidth = widthKnob + (widthDepth * inputs[WIDTHCV_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;

        const float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        float scaledPhasor = normalizedPhasor * numSteps;
        int currentIndex = floorf(scaledPhasor);
        float fractionalIndex = scaledPhasor - floorf(scaledPhasor);

        bool reversePhasor = slopeDetectors[i](normalizedPhasor) < 0.0f;
        bool isZero = scaledPhasor == 0.0f;

        if(slopeDetectors[i].isPhasorAdvancing() || !isZero)
        {
            float gate;
            if (reversePhasor) gate = (1.0f - fractionalIndex) < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;
            else gate = fractionalIndex < pulseWidth ? HCV_PHZ_GATESCALE : 0.0f;

            outputs[GATES_OUTPUT].setVoltage(gates[currentIndex] ? gate : 0.0f, i);
        }
        else outputs[GATES_OUTPUT].setVoltage(0.0f, i);

        if(i == 0) lightIndex = currentIndex;
    }

    // Gate buttons
    for (int i = 0; i < 8; i++) 
    {
        if (gateTriggers[i].process(params[GATE_PARAMS + i].getValue())) {
            gates[i] ^= true;
        }
        lights[GATE_LIGHTS + 3 * i + 0].setBrightness(i >= stepsKnob); //red
        lights[GATE_LIGHTS + 3 * i + 1].setBrightness(gates[i]); //green
        lights[GATE_LIGHTS + 3 * i + 2].setSmoothBrightness(lightIndex == i, args.sampleTime); //blue
    }
}

struct PhasorGatesWidget : HCVModuleWidget { PhasorGatesWidget(PhasorGates *module); };

PhasorGatesWidget::PhasorGatesWidget(PhasorGates *module)
{
    setSkinPath("res/PhasorGates.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    int jackX = 49;
    createInputPort(jackX, 248, PhasorGates::PHASOR_INPUT);
    int knobY = 60;
    createParamComboVertical(15, knobY, PhasorGates::WIDTH_PARAM, PhasorGates::WIDTHCV_PARAM, PhasorGates::WIDTHCV_INPUT);
    createParamComboVertical(70, knobY, PhasorGates::STEPS_PARAM, PhasorGates::STEPSCV_PARAM, PhasorGates::STEPSCV_INPUT);
    
    createOutputPort(jackX, 310, PhasorGates::GATES_OUTPUT);

    for (int i = 0; i < PhasorGates::NUM_STEPS; i++)
    {
        int buttonY = 205 + (i/4)*25;
        int buttonX = 15 + 30 * (i%4);
        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(buttonX, buttonY), 
        module, PhasorGates::GATE_PARAMS + i, PhasorGates::GATE_LIGHTS + 3 * i));
    }
    
    
}

Model *modelPhasorGates = createModel<PhasorGates, PhasorGatesWidget>("PhasorGates");
