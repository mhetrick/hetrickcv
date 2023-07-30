#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorDivMult : HCVModule
{
	enum ParamIds
	{
        DIVIDE_PARAM,
        DIVIDECV_PARAM,
        MULTIPLY_PARAM,
        MULTIPLYCV_PARAM,
        MODE_PARAM,
        RESYNC_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        RESET_INPUT,
        RESYNC_INPUT,
        DIVIDECV_INPUT,
        MULTIPLYCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_OUTPUT,
        GATES_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ENUMS(MODE_LIGHT, 3),
        NUM_LIGHTS
	};


    HCVPhasorDivMult divMults[16];
    dsp::SchmittTrigger resetTriggers[16];
    dsp::SchmittTrigger resyncTriggers[16];
    dsp::SchmittTrigger modeTrigger;
    int mode = 0;

	PhasorDivMult()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, PHASOR_OUTPUT);

        configParam(DIVIDE_PARAM, -8.f, 10.f, 1.f, "Divide", "/", 2, 0.5);
		configParam(DIVIDECV_PARAM, -1.0, 1.0, 1.0, "Divide CV Depth");
        //paramQuantities[DIVIDE_PARAM]->snapEnabled = true;

        configParam(MULTIPLY_PARAM, -8.f, 10.f, 1.f, "Multiply", "x", 2, 0.5);
		configParam(MULTIPLYCV_PARAM, -1.0, 1.0, 1.0, "Multiply CV Depth");
        //paramQuantities[MULTIPLY_PARAM]->snapEnabled = true;

        configButton(MODE_PARAM, "Sync Mode");

        configButton(RESYNC_PARAM, "Resync");
        configButton(RESET_PARAM, "Reset");

        configInput(PHASOR_INPUT, "Phasor");
        configInput(RESET_INPUT, "Reset");
        configInput(RESYNC_INPUT, "Resync");
        configInput(DIVIDECV_INPUT, "Divide CV");
        configInput(MULTIPLYCV_INPUT, "Multiply CV");

        configOutput(PHASOR_OUTPUT, "Phasor");
        configOutput(GATES_OUTPUT, "Gate");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {
        mode = 0;
	}
    void onRandomize() override
    {
        mode = round(random::uniform() * 2.0f);
    }

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
        json_object_set_new(rootJ, "mode", json_integer(mode));
		return rootJ;
	}
    void dataFromJson(json_t *rootJ) override
    {
		json_t *modeJ = json_object_get(rootJ, "mode");
		if (modeJ)
            mode = json_integer_value(modeJ);
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorDivMult::process(const ProcessArgs &args)
{
    const int numChannels = setupPolyphonyForAllOutputs();

    const float divideKnob = params[DIVIDE_PARAM].getValue();
    const float divideCVDepth = params[DIVIDECV_PARAM].getValue();

    const float multiplyKnob = params[MULTIPLY_PARAM].getValue();
    const float multiplyCVDepth = params[MULTIPLYCV_PARAM].getValue();

    const float resetButton = params[RESET_PARAM].getValue();
    const float resyncButton = params[RESYNC_PARAM].getValue();

    if (modeTrigger.process(params[MODE_PARAM].getValue())) mode = (mode + 1) % 3;

    for (int i = 0; i < numChannels; i++)
    {
        float divide =  divideKnob + (inputs[DIVIDECV_INPUT].getPolyVoltage(i) * divideCVDepth);
        float division = 0.5f * rack::dsp::approxExp2_taylor5(divide);

        float multiply =  multiplyKnob + (inputs[MULTIPLYCV_INPUT].getPolyVoltage(i) * multiplyCVDepth);
        float multiplication = 0.5f * rack::dsp::approxExp2_taylor5(multiply);

        divMults[i].setDivider(division);
        divMults[i].setMultiplier(multiplication);

        float resetValue = inputs[RESET_INPUT].getPolyVoltage(i) + resetButton;
        float resyncValue = inputs[RESYNC_INPUT].getPolyVoltage(i) + resyncButton;

        if (resetTriggers[i].process(resetValue)) divMults[i].reset();
        if (resyncTriggers[i].process(resyncValue)) divMults[i].resync();

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));
        float speedPhasor;
        switch(mode)
        {
            case 0: speedPhasor = divMults[i].basicSync(normalizedPhasor); break;
            case 1: speedPhasor = divMults[i].modulatedSync(normalizedPhasor); break;
            case 2: speedPhasor = divMults[i].hardSynced(normalizedPhasor); break;
            default: speedPhasor = normalizedPhasor; break;
        }

        float gate = speedPhasor < 0.5f ? HCV_PHZ_GATESCALE : 0.0f;

        outputs[PHASOR_OUTPUT].setVoltage(speedPhasor * HCV_PHZ_UPSCALE, i);
        outputs[GATES_OUTPUT].setVoltage(gate, i);
    }

    for(int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[MODE_LIGHT + i].setBrightness(mode == i ? 1.0f : 0.0f);
    }

}

struct PhasorDivMultWidget : HCVModuleWidget { PhasorDivMultWidget(PhasorDivMult *module); };

PhasorDivMultWidget::PhasorDivMultWidget(PhasorDivMult *module)
{
    setSkinPath("res/PhasorDivMult.svg");
    initializeWidget(module);
    
    int knobY = 85;

    //////PARAMS//////
    createParamComboVertical(15, knobY, PhasorDivMult::DIVIDE_PARAM, PhasorDivMult::DIVIDECV_PARAM, PhasorDivMult::DIVIDECV_INPUT);
    createParamComboVertical(70, knobY, PhasorDivMult::MULTIPLY_PARAM, PhasorDivMult::MULTIPLYCV_PARAM, PhasorDivMult::MULTIPLYCV_INPUT);


    //////INPUTS//////
    int leftX = 21;
    int middleX = 76;
    int rightX = 128;
    int topJackY = 257;
    int bottomJackY = 319;
    createInputPort(leftX, topJackY, PhasorDivMult::PHASOR_INPUT);
    createInputPort(middleX, topJackY, PhasorDivMult::RESET_INPUT);
    createInputPort(rightX, topJackY, PhasorDivMult::RESYNC_INPUT);

    int buttonOffsetX = 4;
    int buttonOffsetY = 20;
    createHCVButtonSmall(middleX + buttonOffsetX, topJackY - buttonOffsetY, PhasorDivMult::RESET_PARAM);
    createHCVButtonSmall(rightX + buttonOffsetX, topJackY - buttonOffsetY, PhasorDivMult::RESYNC_PARAM);
    
    createOutputPort(51, bottomJackY, PhasorDivMult::PHASOR_OUTPUT);
    createOutputPort(103, bottomJackY, PhasorDivMult::GATES_OUTPUT);

    createHCVButtonLarge(132, 145, PhasorDivMult::MODE_PARAM);

    //////BLINKENLIGHTS//////
    int lightX = 122;
    int lightY = 179;
    int lightSpacing = 13;
    for (int i = 0; i < PhasorDivMult::NUM_LIGHTS; i++)
    {
        createHCVRedLight(lightX, lightY + lightSpacing * i, PhasorDivMult::MODE_LIGHT + i);
    }
}

Model *modelPhasorDivMult = createModel<PhasorDivMult, PhasorDivMultWidget>("PhasorDivMult");
