#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

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
        FORCE_INPUT,
        ACTIVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		RANDOM_OUTPUT,
        STEPPED_OUTPUT,
        RANDOMPHASE_OUTPUT,
        RANDOMGATE_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        ENUMS(MODE_LIGHTS, 6),
        FORCE_LIGHT,
        ACTIVE_LIGHT,
        OUT_LIGHT,
        RANDOM_PHASE_LIGHT,
        RANDOM_GATE_LIGHT,
        STEPPED_LIGHT,
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

	PhasorRandom()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, RANDOM_OUTPUT);
		configParam(CHANCE_PARAM, 0.0, 5.0, 0.0, "Chance");
		configParam(CHANCE_SCALE_PARAM, -1.0, 1.0, 1.0, "Chance CV Depth");

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 1.0, "Steps");
		configParam(STEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;
        
        configSwitch(PhasorRandom::MODE_PARAM, 0.0, 5.0, 0.0, "Mode",
        {"Random Slice", "Random Reverse Slice", "Random Reverse Phasor", "Random Slope", "Random Stutter", "Random Freeze"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
		configParam(PhasorRandom::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");


        configInput(PHASOR_INPUT, "Phasor");
        configInput(FORCE_INPUT, "Force Randomization");

        configInput(CHANCE_INPUT, "Chance CV");
        configInput(STEPS_INPUT, "Steps CV");
        configInput(MODE_INPUT, "Mode CV");
        configInput(ACTIVE_INPUT, "Activation Gate");

        configOutput(RANDOM_OUTPUT, "Randomized Phasor");
        configOutput(STEPPED_OUTPUT, "Stepped Phasor");
        configOutput(RANDOMPHASE_OUTPUT, "Random Phasors");
        configOutput(RANDOMGATE_OUTPUT, "Random Gates");

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
    float stepsCVDepth = params[STEPS_SCALE_PARAM].getValue() * STEPS_CV_SCALE;

    float chanceKnob = params[CHANCE_PARAM].getValue();
    float chanceCVDepth = params[CHANCE_SCALE_PARAM].getValue();

    float modeKnob = params[MODE_PARAM].getValue();
    float modeCVDepth = params[MODE_SCALE_PARAM].getValue();

    for(int i = 0; i < numChannels; i++)
    {
        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        float probability = chanceKnob + (chanceCVDepth * inputs[CHANCE_INPUT].getPolyVoltage(i));
        probability = clamp(probability, 0.0f, 5.0f) * 0.2f;

        float steps = stepsKnob + (stepsCVDepth * inputs[STEPS_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));

        float mode = modeKnob + (modeCVDepth * inputs[MODE_INPUT].getPolyVoltage(i));
        mode = floorf(clamp(mode, 0.0f, 5.0f));

        bool forceRandom = inputs[FORCE_INPUT].getPolyVoltage(i) > 1.0f;

        bool active = true;
        if(inputs[ACTIVE_INPUT].isConnected())
        {
            active = inputs[ACTIVE_INPUT].getPolyVoltage(i) >= 1.0f;
        }

        if(!active) forceRandom = false;

        randomizers[i].setProbability(active ? probability : 0.0f);
        randomizers[i].setNumSteps(steps);
        randomizers[i].setMode(mode);
        randomizers[i].enableForceRandomization(forceRandom);

        float outputPhasor = randomizers[i](normalizedPhasor);

        outputs[RANDOM_OUTPUT].setVoltage(outputPhasor * HCV_PHZ_UPSCALE, i);
        outputs[STEPPED_OUTPUT].setVoltage(randomizers[i].getSteppedPhasor() * HCV_PHZ_UPSCALE, i);
        outputs[RANDOMPHASE_OUTPUT].setVoltage(randomizers[i].getRandomPhasor() * HCV_PHZ_UPSCALE, i);
        outputs[RANDOMGATE_OUTPUT].setVoltage(randomizers[i].getRandomGate(), i);
    }

    int lightMode = modeKnob + modeCVDepth*inputs[MODE_INPUT].getVoltage();
    lightMode = clamp(lightMode, 0, 5);

    for(int i = 0; i < 6; i++)
    {
        lights[i].setBrightness(i == lightMode ? 5.0f : 0.0f);
    }

    bool active = true;
    if(inputs[ACTIVE_INPUT].isConnected())
    {
        active = inputs[ACTIVE_INPUT].getVoltage() >= 1.0f;
    }
    bool forceRandom = inputs[FORCE_INPUT].getVoltage() > 1.0f;

    lights[FORCE_LIGHT].setBrightness(forceRandom ? 1.0f : 0.0f);
    lights[ACTIVE_LIGHT].setBrightness(active ? 1.0f : 0.0f);
    

    setLightFromOutput(OUT_LIGHT, RANDOM_OUTPUT);
    setLightFromOutput(RANDOM_PHASE_LIGHT, RANDOMPHASE_OUTPUT);
    setLightSmoothFromOutput(RANDOM_GATE_LIGHT, RANDOMGATE_OUTPUT);
    setLightFromOutput(STEPPED_LIGHT, STEPPED_OUTPUT);
}


struct PhasorRandomWidget : HCVModuleWidget { PhasorRandomWidget(PhasorRandom *module); };

PhasorRandomWidget::PhasorRandomWidget(PhasorRandom *module)
{
	setSkinPath("res/PhasorRandom.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 39.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorRandom::STEPS_PARAM, PhasorRandom::STEPS_SCALE_PARAM, PhasorRandom::STEPS_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorRandom::CHANCE_PARAM, PhasorRandom::CHANCE_SCALE_PARAM, PhasorRandom::CHANCE_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorRandom::MODE_PARAM, PhasorRandom::MODE_SCALE_PARAM, PhasorRandom::MODE_INPUT);

    const float inJackY = 258.0f;
    const float outJackY = 318.0f;
    const float jack1 = 13.0f;
    const float jack2 = 55.0f;
    const float jack3 = 97.0f;
    const float jack4 = 139.0f;
	//////INPUTS//////
    createInputPort(25, inJackY, PhasorRandom::PHASOR_INPUT);
    createInputPort(78, inJackY, PhasorRandom::FORCE_INPUT);
    createInputPort(131, inJackY, PhasorRandom::ACTIVE_INPUT);

	//////OUTPUTS//////
    createOutputPort(jack1, outJackY, PhasorRandom::RANDOM_OUTPUT);
    createOutputPort(jack2, outJackY, PhasorRandom::RANDOMPHASE_OUTPUT);
    createOutputPort(jack3, outJackY, PhasorRandom::RANDOMGATE_OUTPUT);
    createOutputPort(jack4, outJackY, PhasorRandom::STEPPED_OUTPUT);


    for (int i = 0; i < 6; i++)
    {
        createHCVRedLight(100.0, 188 + (i*9.5), i);
    }

    createHCVRedLightForJack(78, inJackY, PhasorRandom::FORCE_LIGHT);
    createHCVRedLightForJack(131, inJackY, PhasorRandom::ACTIVE_LIGHT);
    
    createHCVRedLightForJack(jack1, outJackY, PhasorRandom::OUT_LIGHT);
    createHCVRedLightForJack(jack2, outJackY, PhasorRandom::RANDOM_PHASE_LIGHT);
    createHCVRedLightForJack(jack3, outJackY, PhasorRandom::RANDOM_GATE_LIGHT);
    createHCVRedLightForJack(jack4, outJackY, PhasorRandom::STEPPED_LIGHT);
    
}

Model *modelPhasorRandom = createModel<PhasorRandom, PhasorRandomWidget>("PhasorRandom");
