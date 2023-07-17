#include "HetrickCV.hpp"
#include "DSP/HCVPhasorEffects.h"

struct PhasorRandom : HCVModule
{
	enum ParamIds
	{
		STEPS_PARAM, STEPS_SCALE_PARAM,
        CHANCE_PARAM, CHANCE_SCALE_PARAM,
        MODE_PARAM, MODE_SCALE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPS_INPUT,
        CHANCE_INPUT,
        MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		RANDOM_OUTPUT,
        STEPPED_OUTPUT,
        CLOCK_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        NUM_LIGHTS = 7
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

	PhasorRandom()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CHANCE_PARAM, 0.0, 5.0, 0.0, "Chance");
		configParam(CHANCE_SCALE_PARAM, -1.0, 1.0, 1.0, "Chance CV Depth");

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 0.0, "Steps");
		configParam(STEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;
        
        configSwitch(PhasorRandom::MODE_PARAM, 0.0, 6.0, 0.0, "Mode",
        {"Random Slice", "Random Reverse Slice", "Random Reverse Phasor", "Random Slope", "Random Stutter", "Random Freeze", "Jitter"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
		configParam(PhasorRandom::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");


        configInput(PHASOR_INPUT, "Phasor");

        configInput(CHANCE_INPUT, "Chance CV");
        configInput(STEPS_INPUT, "Steps CV");
        configInput(MODE_INPUT, "Mode CV");

        configOutput(RANDOM_OUTPUT, "Randomized Phasor");
        configOutput(STEPPED_OUTPUT, "Stepped Phasor");
        configOutput(CLOCK_OUTPUT, "Step Clock");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    HCVPhasorRandomizer randomizers[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorRandom::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPS_SCALE_PARAM].getValue();

    float chanceKnob = params[CHANCE_PARAM].getValue();
    float chanceCVDepth = params[CHANCE_SCALE_PARAM].getValue();

    float modeKnob = params[MODE_PARAM].getValue();
    float modeCVDepth = params[MODE_SCALE_PARAM].getValue();

    for(int i = 0; i < numChannels; i++)
    {
        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        float probability = chanceKnob + (chanceCVDepth * inputs[CHANCE_INPUT].getPolyVoltage(i));
        probability = clamp(probability, 0.0f, 5.0f) * 0.2f;

        float steps = stepsKnob + (stepsCVDepth * inputs[STEPS_INPUT].getPolyVoltage(i) * STEPS_CV_SCALE);
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));

        float mode = modeKnob + (modeCVDepth * inputs[MODE_INPUT].getPolyVoltage(i));
        mode = floorf(clamp(mode, 0.0f, 6.0f));

        randomizers[i].setProbability(probability);
        randomizers[i].setNumSteps(steps);
        randomizers[i].setMode(mode);

        float outputPhasor = randomizers[i](normalizedPhasor);

        outputs[RANDOM_OUTPUT].setVoltage(outputPhasor * HCV_PHZ_UPSCALE, i);
        outputs[STEPPED_OUTPUT].setVoltage(randomizers[i].getSteppedPhasor() * HCV_PHZ_UPSCALE, i);
        outputs[CLOCK_OUTPUT].setVoltage(randomizers[i].getGateOutput(), i);
    }

    int lightMode = modeKnob + modeCVDepth*inputs[MODE_INPUT].getVoltage();
    lightMode = clamp(lightMode, 0, 6);

    for(int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[i].setBrightness(i == lightMode ? 5.0f : 0.0f);
    }
}


struct PhasorRandomWidget : HCVModuleWidget { PhasorRandomWidget(PhasorRandom *module); };

PhasorRandomWidget::PhasorRandomWidget(PhasorRandom *module)
{
	setSkinPath("res/PhasorRandom.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorRandom::STEPS_PARAM, PhasorRandom::STEPS_SCALE_PARAM, PhasorRandom::STEPS_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorRandom::CHANCE_PARAM, PhasorRandom::CHANCE_SCALE_PARAM, PhasorRandom::CHANCE_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorRandom::MODE_PARAM, PhasorRandom::MODE_SCALE_PARAM, PhasorRandom::MODE_INPUT);


    const float switchY = 238.0f;

    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(11.0f, jackY, PhasorRandom::PHASOR_INPUT);

	//////OUTPUTS//////
    createOutputPort(62.0f, jackY, PhasorRandom::RANDOM_OUTPUT);
    createOutputPort(104.0f, jackY, PhasorRandom::STEPPED_OUTPUT);
    createOutputPort(146.0f, jackY, PhasorRandom::CLOCK_OUTPUT);


    for (int i = 0; i < PhasorRandom::NUM_LIGHTS; i++)
    {
        createHCVRedLight(105.0, 223 + (i*9.5), i);
    }
    
}

Model *modelPhasorRandom = createModel<PhasorRandom, PhasorRandomWidget>("PhasorRandom");
