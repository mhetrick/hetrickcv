#include "HetrickCV.hpp"

struct BinaryCounter : HCVModule
{
	enum ParamIds
	{
        ADD_PARAM,
        SUBTRACT_PARAM,
        RESET_PARAM,

        INV1_PARAM,
        INV2_PARAM,
        INV3_PARAM,
        INV4_PARAM,
        INV5_PARAM,
        INV6_PARAM,
        INV7_PARAM,
        INV8_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        IN7_INPUT,
        IN8_INPUT,

        ADD_INPUT,
        SUBTRACT_INPUT,
        RESET_INPUT,

        POLY_INPUT,

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
        POLY_OUTPUT,

		NUM_OUTPUTS
    };
    enum LightIds
	{
        IN1_LIGHT,
        IN2_LIGHT,
        IN3_LIGHT,
        IN4_LIGHT,
        IN5_LIGHT,
        IN6_LIGHT,
        IN7_LIGHT,
        IN8_LIGHT,

        OUT1_LIGHT,
        OUT2_LIGHT,
        OUT3_LIGHT,
        OUT4_LIGHT,
        OUT5_LIGHT,
        OUT6_LIGHT,
        OUT7_LIGHT,
        OUT8_LIGHT,

        INV1_LIGHT,
        INV2_LIGHT,
        INV3_LIGHT,
        INV4_LIGHT,
        INV5_LIGHT,
        INV6_LIGHT,
        INV7_LIGHT,
        INV8_LIGHT,

        ADD_LIGHT,
        SUBTRACT_LIGHT,
        RESET_LIGHT,

		NUM_LIGHTS
    };

    dsp::SchmittTrigger addTrigger, subtractTrigger, resetTrigger;
    dsp::SchmittTrigger invTrigger[8];

    bool ins[8] = {};
    float outs[8] = {};
    bool invState[8] = {};

    uint8_t counter = 0, step = 1;

	BinaryCounter()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 8; i++)
        {
            const auto channelString = std::to_string(i + 1);
            configInput(IN1_INPUT + i, "Bit " + channelString);
            configOutput(OUT1_OUTPUT + i, "Bit " + channelString);
            configButton(INV1_PARAM + i, "Invert " + channelString);
        }

        configButton(ADD_PARAM, "Add");
        configButton(SUBTRACT_PARAM, "Subtract");
        configButton(RESET_PARAM, "Reset");
    
        configInput(ADD_INPUT, "Add");
        configInput(SUBTRACT_INPUT, "Subtract");
        configInput(RESET_INPUT, "Reset");

        configInput(POLY_INPUT, "Poly");
        configOutput(POLY_OUTPUT, "Poly");

        invState[0] = true;
	}

    void process(const ProcessArgs &args) override;

    void calculateStep();

    void onReset() override
    {
        for (int i = 1; i < 8; i++)
        {
            invState[i] = false;
		}

        invState[0] = true;
	}
    void onRandomize() override
    {
        for (int i = 0; i < 8; i++)
        {
            invState[i] = (random::uniform() < 0.5);
		}
    }

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
		// states
        json_t *invStatesJ = json_array();
        for (int i = 0; i < 8; i++)
        {
            json_t *invStateJ = json_boolean(invState[i]);
			json_array_append_new(invStatesJ, invStateJ);
		}
        json_object_set_new(rootJ, "invStates", invStatesJ);
		return rootJ;
	}
    void dataFromJson(json_t *rootJ) override
    {
        // states
        json_t *invStatesJ = json_object_get(rootJ, "invStates");
        if (invStatesJ)
        {
            for (int i = 0; i < 8; i++)
            {
				json_t *stateJ = json_array_get(invStatesJ, i);
				if (stateJ)
					invState[i] = json_boolean_value(stateJ);
			}
		}
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void BinaryCounter::calculateStep()
{
    step = 0;
    if(ins[0]) step += 1;
    if(ins[1]) step += 2;
    if(ins[2]) step += 4;
    if(ins[3]) step += 8;
    if(ins[4]) step += 16;
    if(ins[5]) step += 32;
    if(ins[6]) step += 64;
    if(ins[7]) step += 128;

    //fancy chatGPT way
    // for (int i = 0; i < 8; i++) 
    // {
    //     if (ins[i]) 
    //     {
    //         step |= (1 << i);
    //     }
    // }
}

void BinaryCounter::process(const ProcessArgs &args)
{
    inputs[POLY_INPUT].setChannels(8);

    for(int i = 0; i < 8; i++)
    {
        ins[i] = inputs[POLY_INPUT].getVoltage(i) >= 1.0f;
        if (inputs[IN1_INPUT + i].isConnected()) ins[i] = inputs[IN1_INPUT + i].getVoltage() >= 1.0f;

        bool invertGate = params[INV1_PARAM + i].getValue() >= 1.0f;
        if (invTrigger[i].process(invertGate ? HCV_GATE_MAG : 0.0f)) invState[i] ^= true;

        if(invState[i]) ins[i] = !ins[i];
    }

    calculateStep();

    float addState = inputs[ADD_INPUT].getVoltage() + params[ADD_PARAM].getValue();
    float subtractState = inputs[SUBTRACT_INPUT].getVoltage() + params[SUBTRACT_PARAM].getValue();
    float resetState = inputs[RESET_INPUT].getVoltage() + params[RESET_PARAM].getValue();

    if(addTrigger.process(addState))
    {
        counter += step;
    }

    if(subtractTrigger.process(subtractState))
    {
        counter -= step;
    }

    if(resetTrigger.process(resetState))
    {
        counter = 0;
    }

    outs[0] = (counter & 0b00000001) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[1] = (counter & 0b00000010) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[2] = (counter & 0b00000100) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[3] = (counter & 0b00001000) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[4] = (counter & 0b00010000) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[5] = (counter & 0b00100000) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[6] = (counter & 0b01000000) > 0.0f ? HCV_GATE_MAG : 0.0f;
    outs[7] = (counter & 0b10000000) > 0.0f ? HCV_GATE_MAG : 0.0f;

    for(int i = 0; i < 8; i++)
    {
        outputs[OUT1_OUTPUT + i].setVoltage(outs[i]);
        outputs[POLY_OUTPUT].setVoltage(outs[i], i);

        lights[IN1_LIGHT + i].value = ins[i] ? 1.0f : 0.0f;
        lights[OUT1_LIGHT + i].value = outs[i];
        lights[INV1_LIGHT + i].setBrightness(invState[i] ? 0.9 : 0.0);
    }

    lights[ADD_LIGHT].value = addTrigger.isHigh() ? 1.0f : 0.0f;
    lights[SUBTRACT_LIGHT].value = subtractTrigger.isHigh() ? 1.0f : 0.0f;
    lights[RESET_LIGHT].value = resetTrigger.isHigh() ? 1.0f : 0.0f;

    outputs[POLY_OUTPUT].setChannels(8);
}

struct BinaryCounterWidget : HCVModuleWidget { BinaryCounterWidget(BinaryCounter *module); };

BinaryCounterWidget::BinaryCounterWidget(BinaryCounter *module)
{
    setSkinPath("res/BinaryCounter.svg");
    initializeWidget(module);

    //////PARAMS//////

    //////BLINKENLIGHTS//////

    

    //////INPUTS//////

    const int inXPos = 10;
    const int inLightX = 50;
    const int outXPos = 175;
    const int outLightX = 150;
    const int invParamX = 64;
    for(int i = 0; i < 8; i++)
    {
        const int yPos = 50 + (40 * i);
        const int lightY = 59 + (40 * i);

        //////INPUTS//////
        addInput(createInput<PJ301MPort>(Vec(inXPos, yPos), module, BinaryCounter::IN1_INPUT + i));

        //////OUTPUTS//////
        addOutput(createOutput<PJ301MPort>(Vec(outXPos, yPos), module, BinaryCounter::OUT1_OUTPUT + i));

        //////BLINKENLIGHTS//////
        createHCVRedLight(inLightX, lightY, BinaryCounter::IN1_LIGHT + i);
        createHCVRedLight(outLightX, lightY, BinaryCounter::OUT1_LIGHT + i);

        //////PARAMS//////
        addParam(createParam<LEDBezel>(Vec(invParamX, 1 + yPos), module, BinaryCounter::INV1_PARAM + i));
        addChild(createLight<MuteLight<BlueLight>>(Vec(invParamX + 2.2, 3 + yPos), module, BinaryCounter::INV1_LIGHT + i));
    }

    int centerX = 113;

    int addY = 70;
    addInput(createInput<PJ301MPort>(Vec(centerX, addY), module, BinaryCounter::ADD_INPUT));
    createHCVButtonSmallForJack(centerX, addY, BinaryCounter::ADD_PARAM);
    createHCVRedLightForJack(centerX, addY, BinaryCounter::ADD_LIGHT);

    int subtractY = 135;
    addInput(createInput<PJ301MPort>(Vec(centerX, subtractY), module, BinaryCounter::SUBTRACT_INPUT));
    createHCVButtonSmallForJack(centerX, subtractY, BinaryCounter::SUBTRACT_PARAM);
    createHCVRedLightForJack(centerX, subtractY, BinaryCounter::SUBTRACT_LIGHT);

    int resetY = 200;
    addInput(createInput<PJ301MPort>(Vec(centerX, resetY), module, BinaryCounter::RESET_INPUT));
    createHCVButtonSmallForJack(centerX, resetY, BinaryCounter::RESET_PARAM);
    createHCVRedLightForJack(centerX, resetY, BinaryCounter::RESET_LIGHT);

    addInput(createInput<PJ301MPort>(Vec(centerX, 255), module, BinaryCounter::POLY_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(centerX, 310), module, BinaryCounter::POLY_OUTPUT));
}

Model *modelBinaryCounter = createModel<BinaryCounter, BinaryCounterWidget>("BinaryCounter");
