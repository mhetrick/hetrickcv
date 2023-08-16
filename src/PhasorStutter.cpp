#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorStutter : HCVModule
{
	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        REPEATS_PARAM,
        REPEATSCV_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        ACTIVE_INPUT,
        STEPSCV_INPUT,
        REPEATSCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_OUTPUT,
        GATES_OUTPUT,
        STEPPHASOR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ACTIVE_LIGHT,
        PHASOR_LIGHT,
        GATES_LIGHT,
        STEPPHASOR_LIGHT,
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float MAX_REPEATS = 64.0f;
    static constexpr float REPEATS_CV_SCALE = MAX_REPEATS/5.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

    HCVPhasorStepDetector stepDetectors[16];

	PhasorStutter()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, PHASOR_OUTPUT);

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 1.0, "Steps");
		configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(REPEATS_PARAM, 1.0, MAX_REPEATS, 0.0, "Repeats");
		configParam(REPEATSCV_PARAM, -1.0, 1.0, 1.0, "Repeats CV Depth");
        paramQuantities[REPEATS_PARAM]->snapEnabled = true;

        configInput(PHASOR_INPUT, "Phasor");
        configInput(ACTIVE_INPUT, "Activation Gate");
        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(REPEATSCV_INPUT, "Repeats CV");

        configOutput(PHASOR_OUTPUT, "Stuttered Phasor");
        configOutput(GATES_OUTPUT, "Stuttered Gates");
        configOutput(STEPPHASOR_OUTPUT, "Step Phasors");

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


void PhasorStutter::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    float repeatsKnob = params[REPEATS_PARAM].getValue();
    float repeatsCVDepth = params[REPEATSCV_PARAM].getValue() * REPEATS_CV_SCALE;

    for (int i = 0; i < numChannels; i++)
    {
        float steps = stepsKnob + (stepsCVDepth * inputs[STEPSCV_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));
        stepDetectors[i].setNumberSteps(steps);
        float stepFraction = 1.0f/steps;

        float repeats = repeatsKnob + (repeatsCVDepth * inputs[REPEATSCV_INPUT].getPolyVoltage(i));
        repeats = floorf(clamp(repeats, 1.0f, MAX_REPEATS));

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        bool stepAdvanced = stepDetectors[i](normalizedPhasor);
        float offsetStep = stepDetectors[i].getCurrentStep();
        float offsetBase = stepFraction * offsetStep;

        bool active = true;
        if(inputs[ACTIVE_INPUT].isConnected())
        {
            active = inputs[ACTIVE_INPUT].getPolyVoltage(i) >= 1.0f;
        }

        if(active)
        {
            float stutterStep = gam::scl::wrap(stepDetectors[i].getFractionalStep() * stepFraction * repeats, stepFraction, 0.0f);
            float stutterPhasor = offsetBase + stutterStep;

            bool gate = stutterStep < stepFraction * 0.5f;

            outputs[PHASOR_OUTPUT].setVoltage(stutterPhasor * HCV_PHZ_UPSCALE, i);
            outputs[GATES_OUTPUT].setVoltage(gate ? HCV_PHZ_GATESCALE : 0.0f, i);
            outputs[STEPPHASOR_OUTPUT].setVoltage(stutterStep * steps * HCV_PHZ_UPSCALE, i);
        }
        else
        {
            float substep = stepDetectors[i].getFractionalStep();
            bool gate = substep < 0.5f;

            outputs[PHASOR_OUTPUT].setVoltage(normalizedPhasor * HCV_PHZ_UPSCALE, i);
            outputs[GATES_OUTPUT].setVoltage(gate ? HCV_PHZ_GATESCALE : 0.0f, i);
            outputs[STEPPHASOR_OUTPUT].setVoltage(substep * HCV_PHZ_UPSCALE, i);
        }
    }

    bool active = true;
    if(inputs[ACTIVE_INPUT].isConnected())
    {
        active = inputs[ACTIVE_INPUT].getVoltage() >= 1.0f;
    }

    lights[ACTIVE_LIGHT].setBrightness(active ? 1.0f : 0.0f);

    setLightFromOutput(PHASOR_LIGHT, PHASOR_OUTPUT);
    setLightFromOutput(GATES_LIGHT, GATES_OUTPUT);
    setLightFromOutput(STEPPHASOR_LIGHT, STEPPHASOR_OUTPUT);
}

struct PhasorStutterWidget : HCVModuleWidget { PhasorStutterWidget(PhasorStutter *module); };

PhasorStutterWidget::PhasorStutterWidget(PhasorStutter *module)
{
    setSkinPath("res/PhasorStutter.svg");
    initializeWidget(module);
    
    int knobY = 90;

    //////PARAMS//////
    createParamComboVertical(15, knobY, PhasorStutter::STEPS_PARAM, PhasorStutter::STEPSCV_PARAM, PhasorStutter::STEPSCV_INPUT);
    createParamComboVertical(70, knobY, PhasorStutter::REPEATS_PARAM, PhasorStutter::REPEATSCV_PARAM, PhasorStutter::REPEATSCV_INPUT);


    //////INPUTS//////
    int leftX = 21;
    int rightX = 76;
    int topJackY = 245;
    int bottomJackY = 310;
    createInputPort(leftX, topJackY, PhasorStutter::PHASOR_INPUT);
    createInputPort(rightX, topJackY, PhasorStutter::ACTIVE_INPUT);
    
    createOutputPort(12, bottomJackY, PhasorStutter::PHASOR_OUTPUT);
    createOutputPort(48, bottomJackY, PhasorStutter::STEPPHASOR_OUTPUT);
    createOutputPort(84, bottomJackY, PhasorStutter::GATES_OUTPUT);

    createHCVRedLightForJack(rightX, topJackY, PhasorStutter::ACTIVE_LIGHT);
    
    createHCVRedLightForJack(12, bottomJackY, PhasorStutter::PHASOR_LIGHT);
    createHCVRedLightForJack(48, bottomJackY, PhasorStutter::STEPPHASOR_LIGHT);
    createHCVRedLightForJack(84, bottomJackY, PhasorStutter::GATES_LIGHT);
}

Model *modelPhasorStutter = createModel<PhasorStutter, PhasorStutterWidget>("PhasorStutter");
