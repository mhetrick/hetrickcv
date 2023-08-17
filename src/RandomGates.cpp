#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h"

struct RandomGates : HCVModule
{
	enum ParamIds
	{
        MIN_PARAM,
        MAX_PARAM,
        MODE_PARAM,
        MODE_PARAM_INVIS,
		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,

        MINI_INPUT,
        MAXI_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
	{
        CLOCK_LIGHT,

        MODE_TRIG_LIGHT,
        MODE_GATE_LIGHT,
        MODE_HOLD_LIGHT,

        OUT1_LIGHT,
        OUT2_LIGHT,
        OUT3_LIGHT,
        OUT4_LIGHT,
        OUT5_LIGHT,
        OUT6_LIGHT,
        OUT7_LIGHT,
        OUT8_LIGHT,

		NUM_LIGHTS
	};

	RandomGates()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configSwitch(RandomGates::MIN_PARAM, 0, 7.0, 0.0, "Minimum Output Channel", {"1", "2", "3", "4", "5", "6", "7", "8"});
        configSwitch(RandomGates::MAX_PARAM, 0, 7.0, 7.0, "Maximum Output Channel", {"1", "2", "3", "4", "5", "6", "7", "8"});
        paramQuantities[MIN_PARAM]->snapEnabled = true;
        paramQuantities[MAX_PARAM]->snapEnabled = true;
        
        configButton(RandomGates::MODE_PARAM, "Output Mode");

        configInput(CLOCK_INPUT, "Clock");
        configInput(MINI_INPUT, "Minimum Output Channel CV");
        configInput(MAXI_INPUT, "Maximum Output Channel CV");

        for (int i = 0; i < 8; i++)
        {
            configOutput(OUT1_OUTPUT + i, "Gate " + std::to_string(i + 1));
        }
	}

    void process(const ProcessArgs &args) override;

    int clampInt(const int _in, const int min = 0, const int max = 7)
    {
        if (_in > max) return max;
        if (_in < min) return min;
        return _in;
    }

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger modeTrigger;

    HCVTriggeredGate trigger[8];
    dsp::SchmittTrigger trigOut[8];

    bool active[8] = {};
    int mode = 0;

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

    void onReset() override
    {
		mode = 0;
    }

    void onRandomize() override
    {
		mode = round(random::uniform() * 2.0f);
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void RandomGates::process(const ProcessArgs &args)
{
    int max = round(params[MAX_PARAM].getValue() + inputs[MAXI_INPUT].getVoltage());
    int min = round(params[MIN_PARAM].getValue() + inputs[MINI_INPUT].getVoltage());

    max = clampInt(max);
    min = clampInt(min);

    if (min > max) min = max;

    const bool clockHigh = inputs[CLOCK_INPUT].getVoltage() > 1.0f;

    if (modeTrigger.process(params[MODE_PARAM].getValue()))
    {
		mode = (mode + 1) % 3;
    }

    if (clockTrigger.process(clockHigh))
    {
        uint32_t range = max-min;
        uint32_t randNum;
        if (range == 0) randNum = max;
        else randNum = (random::u32() % (range + 1)) + min;

        for(uint32_t i = 0; i < 8; i++)
        {
            active[i] = randNum == i;
        }
    }

    lights[MODE_TRIG_LIGHT].setBrightness(mode == 0 ? 1.0f : 0.0f);
    lights[MODE_HOLD_LIGHT].setBrightness(mode == 1 ? 1.0f : 0.0f);
    lights[MODE_GATE_LIGHT].setBrightness(mode == 2 ? 1.0f : 0.0f);

    switch(mode)
    {
        case 0: //trigger mode
        for(int i = 0; i < 8; i++)
        {
            if(trigOut[i].process(active[i]))
            {
                trigger[i].trigger();
                active[i] = false;
            }
            outputs[i].setVoltage((trigger[i].process() ? HCV_GATE_MAG : 0.0f));
        }
        break;

        case 1: //hold mode
        for(int i = 0; i < 8; i++)
        {
            outputs[i].setVoltage((active[i] ? HCV_GATE_MAG : 0.0f));
        }
        break;

        case 2: //gate mode
        for(int i = 0; i < 8; i++)
        {
            outputs[i].setVoltage(((active[i] && clockHigh) ? HCV_GATE_MAG : 0.0f));
        }
        break;

        default:
        break;
    }

    for(int i = 0; i < 8; i++)
    {
        lights[OUT1_LIGHT + i].setBrightnessSmooth(fmaxf(0.0, outputs[i].getVoltage()), args.sampleTime * 4.0f);
    }
}


struct RandomGatesWidget : HCVModuleWidget { RandomGatesWidget(RandomGates *module); };

RandomGatesWidget::RandomGatesWidget(RandomGates *module)
{
    setSkinPath("res/RandomGates.svg");
    initializeWidget(module);
    
    
    const int outXPos = 145;
    const int outLightX = 120;
    const int inLightX = 45;
    const int inJackX = 58;

    createInputPort(inJackX, 90, RandomGates::CLOCK_INPUT);
    createInputPort(inJackX, 150, RandomGates::MINI_INPUT);
    createInputPort(inJackX, 210, RandomGates::MAXI_INPUT);
    
    createHCVKnob(10, 145, RandomGates::MIN_PARAM);
    createHCVKnob(10, 205, RandomGates::MAX_PARAM);

    createHCVButtonLarge(56, 270, RandomGates::MODE_PARAM);

    //////BLINKENLIGHTS//////
    addChild(createLight<SmallLight<RedLight>>(Vec(inLightX, 306), module, RandomGates::MODE_TRIG_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(inLightX, 319), module, RandomGates::MODE_HOLD_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(inLightX, 332), module, RandomGates::MODE_GATE_LIGHT));

    for(int i = 0; i < 8; i++)
    {
        const int yPos = 50 + (40 * i);
        const int lightY = 59 + (40 * i);

        //////OUTPUTS//////
        addOutput(createOutput<PJ301MPort>(Vec(outXPos, yPos), module, i));

        //////BLINKENLIGHTS//////
        addChild(createLight<SmallLight<RedLight>>(Vec(outLightX, lightY), module, RandomGates::OUT1_LIGHT + i));
    }
}

Model *modelRandomGates = createModel<RandomGates, RandomGatesWidget>("RandomGates");
