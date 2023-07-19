#include "HetrickCV.hpp"

struct GateJunctionExp : HCVModule
{
	enum ParamIds
	{
        MUTE1_PARAM,
        MUTE2_PARAM,
        MUTE3_PARAM,
        MUTE4_PARAM,
        MUTE5_PARAM,
        MUTE6_PARAM,
        MUTE7_PARAM,
        MUTE8_PARAM,

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

        MUTE1_INPUT,
        MUTE2_INPUT,
        MUTE3_INPUT,
        MUTE4_INPUT,
        MUTE5_INPUT,
        MUTE6_INPUT,
        MUTE7_INPUT,
        MUTE8_INPUT,

        INVERT1_INPUT,
        INVERT2_INPUT,
        INVERT3_INPUT,
        INVERT4_INPUT,
        INVERT5_INPUT,
        INVERT6_INPUT,
        INVERT7_INPUT,
        INVERT8_INPUT,

        POLY_INPUT,
        POLY_MUTE_INPUT,
        POLY_INVERT_INPUT,

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
        MUTE1_LIGHT,
        MUTE2_LIGHT,
        MUTE3_LIGHT,
        MUTE4_LIGHT,
        MUTE5_LIGHT,
        MUTE6_LIGHT,
        MUTE7_LIGHT,
        MUTE8_LIGHT,

        INV1_LIGHT,
        INV2_LIGHT,
        INV3_LIGHT,
        INV4_LIGHT,
        INV5_LIGHT,
        INV6_LIGHT,
        INV7_LIGHT,
        INV8_LIGHT,

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

    float ins[8] = {};
    float outs[8] = {};

    bool muteState[8] = {};
    dsp::SchmittTrigger muteTrigger[8];

    bool invState[8] = {};
	dsp::SchmittTrigger invTrigger[8];

	GateJunctionExp()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 8; ++i)
        {
            const auto channelString = std::to_string(i + 1);

            configBypass(GateJunctionExp::IN1_INPUT + i, GateJunctionExp::OUT1_OUTPUT + i);
            configButton(GateJunctionExp::MUTE1_PARAM + i, "Mute " + channelString);
            configButton(GateJunctionExp::INV1_PARAM + i, "Invert " + channelString);

            configInput(GateJunctionExp::IN1_INPUT + i, "Gate " + channelString);
            configInput(GateJunctionExp::MUTE1_INPUT + i, "Mute " + channelString);
            configInput(GateJunctionExp::INVERT1_INPUT + i, "Invert " + channelString);

            configOutput(GateJunctionExp::OUT1_OUTPUT + i, "Gate " + channelString);
        }

        configInput(GateJunctionExp::POLY_INPUT, "Polyphonic Gate");
        configInput(GateJunctionExp::POLY_MUTE_INPUT, "Polyphonic Mute");
        configInput(GateJunctionExp::POLY_INVERT_INPUT, "Polyphonic Invert");

        configOutput(GateJunctionExp::POLY_OUTPUT, "Polyphonic Gate");

		onReset();
	}

    void process(const ProcessArgs &args) override;

    void onReset() override
    {
        for (int i = 0; i < 8; i++)
        {
            muteState[i] = false;
            invState[i] = false;
		}
	}
    void onRandomize() override
    {
        for (int i = 0; i < 8; i++)
        {
            muteState[i] = (random::uniform() < 0.5);
            invState[i] = (random::uniform() < 0.5);
		}
    }

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
		// states
        json_t *muteStatesJ = json_array();
        json_t *invStatesJ = json_array();
        for (int i = 0; i < 8; i++)
        {
			json_t *muteStateJ = json_boolean(muteState[i]);
            json_array_append_new(muteStatesJ, muteStateJ);
            json_t *invStateJ = json_boolean(invState[i]);
			json_array_append_new(invStatesJ, invStateJ);
		}
        json_object_set_new(rootJ, "muteStates", muteStatesJ);
        json_object_set_new(rootJ, "invStates", invStatesJ);
		return rootJ;
	}
    void dataFromJson(json_t *rootJ) override
    {
		// states
        json_t *muteStatesJ = json_object_get(rootJ, "muteStates");
        json_t *invStatesJ = json_object_get(rootJ, "invStates");
        if (muteStatesJ)
        {
            for (int i = 0; i < 8; i++)
            {
				json_t *stateJ = json_array_get(muteStatesJ, i);
				if (stateJ)
					muteState[i] = json_boolean_value(stateJ);
			}
        }
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


void GateJunctionExp::process(const ProcessArgs &args)
{
    inputs[POLY_INPUT].setChannels(8);
    inputs[POLY_MUTE_INPUT].setChannels(8);
    inputs[POLY_INVERT_INPUT].setChannels(8);
    outputs[POLY_OUTPUT].setChannels(8);

    for(int i = 0; i < 8; i++)
    {
        bool gateActive = (inputs[IN1_INPUT + i].getVoltage() >= 1.0f) || (inputs[POLY_INPUT].getVoltage(i) >= 1.0f);
        ins[i] = gateActive ? HCV_GATE_MAG : 0.0f;

        bool muteGate = params[MUTE1_PARAM + i].getValue() >= 1.0f;
        muteGate = muteGate || inputs[MUTE1_INPUT + i].getVoltage() >= 1.0f;
        muteGate = muteGate || inputs[POLY_MUTE_INPUT].getVoltage(i) >= 1.0f;
        
        bool invertGate = params[INV1_PARAM + i].getValue() >= 1.0f;
        invertGate = invertGate || inputs[INVERT1_INPUT + i].getVoltage() >= 1.0f;                   
        invertGate = invertGate || inputs[POLY_INVERT_INPUT].getVoltage(i) >= 1.0f;

        if (muteTrigger[i].process(muteGate  ? HCV_GATE_MAG : 0.0f)) muteState[i] ^= true;
        if (invTrigger[i].process(invertGate ? HCV_GATE_MAG : 0.0f)) invState[i] ^= true;

        if(muteState[i]) ins[i] = 0.0f;
        else if(invState[i]) ins[i] = HCV_GATE_MAG - ins[i];

        outputs[OUT1_OUTPUT + i].setVoltage(ins[i]);
        outputs[POLY_OUTPUT].setVoltage(ins[i], i);

        lights[OUT1_LIGHT + i].setBrightnessSmooth(ins[i], args.sampleTime * 4.0f);

        lights[MUTE1_LIGHT + i].setBrightness(muteState[i] ? 0.9 : 0.0);
        lights[INV1_LIGHT + i].setBrightness(invState[i] ? 0.9 : 0.0);
    }
}

template <typename BASE>
struct MuteLight : BASE {
	MuteLight() {
		this->box.size = mm2px(Vec(6.0, 6.0));
	}
};

struct GateJunctionExpWidget : HCVModuleWidget { GateJunctionExpWidget(GateJunctionExp *module); };

GateJunctionExpWidget::GateJunctionExpWidget(GateJunctionExp *module)
{
    setSkinPath("res/GateJunctionExpanded.svg");
    initializeWidget(module);

    //////PARAMS//////

    const int inXPos = 80;
    const int outXPos = 293;
    const int outLightX = outXPos - 25;

    float muteParamX = 155;
    float invParamX = 230;
    for(int i = 0; i < 8; i++)
    {
        const int yPos = 50 + (40 * i);
        const int lightY = 59 + (40 * i);

        //////INPUTS//////
        addInput(createInput<PJ301MPort>(Vec(inXPos, yPos), module, GateJunctionExp::IN1_INPUT + i));
        addInput(createInput<PJ301MPort>(Vec(muteParamX - 40, yPos), module, GateJunctionExp::MUTE1_INPUT + i));
        addInput(createInput<PJ301MPort>(Vec(invParamX - 40, yPos), module, GateJunctionExp::INVERT1_INPUT + i));

        //////OUTPUTS//////
        addOutput(createOutput<PJ301MPort>(Vec(outXPos, yPos), module, GateJunctionExp::OUT1_OUTPUT + i));

        //////BLINKENLIGHTS//////
        addChild(createLight<SmallLight<RedLight>>(Vec(outLightX, lightY), module, GateJunctionExp::OUT1_LIGHT + i));

        addParam(createParam<LEDBezel>(Vec(muteParamX, 1 + yPos), module, GateJunctionExp::MUTE1_PARAM + i));
        addParam(createParam<LEDBezel>(Vec(invParamX, 1 + yPos), module, GateJunctionExp::INV1_PARAM + i));

        addChild(createLight<MuteLight<RedLight>>(Vec(muteParamX + 2.2, 3 + yPos), module, GateJunctionExp::MUTE1_LIGHT + i));
        addChild(createLight<MuteLight<BlueLight>>(Vec(invParamX + 2.2, 3 + yPos), module, GateJunctionExp::INV1_LIGHT + i));
    }

    const int polyX = 22;
    addInput(createInput<PJ301MPort>(Vec(polyX, 70), module, GateJunctionExp::POLY_INPUT));
    addInput(createInput<PJ301MPort>(Vec(polyX, 150), module, GateJunctionExp::POLY_MUTE_INPUT));
    addInput(createInput<PJ301MPort>(Vec(polyX, 230), module, GateJunctionExp::POLY_INVERT_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(polyX, 310), module, GateJunctionExp::POLY_OUTPUT));
}

Model *modelGateJunctionExp = createModel<GateJunctionExp, GateJunctionExpWidget>("GateJunctionExp");
