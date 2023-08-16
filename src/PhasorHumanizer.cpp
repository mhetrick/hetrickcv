#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorHumanizer : HCVModule
{
	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        HUMANIZE_PARAM,
        HUMANIZECV_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        HUMANIZECV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

    HCVPhasorHumanizer humanizers[16];

	PhasorHumanizer()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, PHASOR_OUTPUT);

        configParam(STEPS_PARAM, 2.0f, MAX_STEPS, 2.0f, "Steps");
		configParam(STEPSCV_PARAM, -1.0f, 1.0f, 1.0f, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(HUMANIZE_PARAM, 0.0f, 5.0f, 0.0f, "Humanize");
		configParam(HUMANIZECV_PARAM, -1.0f, 1.0f, 1.0f, "Humanize CV Depth");

        configInput(PHASOR_INPUT, "Phasor");
        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(HUMANIZECV_INPUT, "Humanize CV");

        configOutput(PHASOR_OUTPUT, "Humanized Phasor");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {

    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorHumanizer::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    float humanizeKnob = params[HUMANIZE_PARAM].getValue();
    float humanizeCVDepth = params[HUMANIZECV_PARAM].getValue();

    for (int i = 0; i < numChannels; i++)
    {
        float steps = stepsKnob + (stepsCVDepth * inputs[STEPSCV_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 2.0f, MAX_STEPS));
        humanizers[i].setNumSteps(steps);

        float humanize = humanizeKnob + (humanizeCVDepth * inputs[HUMANIZECV_INPUT].getPolyVoltage(i));
        humanize = clamp(humanize, 0.0f, 5.0f) * 0.2f;
        humanizers[i].setDepth(humanize);

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));
        float humanizedPhasor = humanizers[i](normalizedPhasor);

        outputs[PHASOR_OUTPUT].setVoltage(humanizedPhasor * HCV_PHZ_UPSCALE, i);
    }
}

struct PhasorHumanizerWidget : HCVModuleWidget { PhasorHumanizerWidget(PhasorHumanizer *module); };

PhasorHumanizerWidget::PhasorHumanizerWidget(PhasorHumanizer *module)
{
    setSkinPath("res/PhasorHumanizer.svg");
    initializeWidget(module);
    
    int knobY = 90;

    //////PARAMS//////
    createParamComboVertical(15, knobY, PhasorHumanizer::STEPS_PARAM, PhasorHumanizer::STEPSCV_PARAM, PhasorHumanizer::STEPSCV_INPUT);
    createParamComboVertical(70, knobY, PhasorHumanizer::HUMANIZE_PARAM, PhasorHumanizer::HUMANIZECV_PARAM, PhasorHumanizer::HUMANIZECV_INPUT);


    //////INPUTS//////
    int leftX = 21;
    int rightX = 76;
    int topJackY = 245;
    int bottomJackY = 310;
    createInputPort(leftX, bottomJackY, PhasorHumanizer::PHASOR_INPUT);
    createOutputPort(rightX, bottomJackY, PhasorHumanizer::PHASOR_OUTPUT);
}

Model *modelPhasorHumanizer = createModel<PhasorHumanizer, PhasorHumanizerWidget>("PhasorHumanizer");
