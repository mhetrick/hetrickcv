#include "HetrickCV.hpp"

struct LogicCombine : HCVModule
{
	enum ParamIds
	{
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
        POLY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OR_OUTPUT,
        NOR_OUTPUT,
        TRIG_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        OR_LIGHT,
        NOR_LIGHT,
		TRIG_LIGHT,
        NUM_LIGHTS
	};

    static constexpr int totalInputs = 24;
    bool ins[totalInputs] = {};
    bool trigs[totalInputs] = {};
    dsp::SchmittTrigger inTrigs[totalInputs];

    TriggerGenWithSchmitt triggerProcessor;

    float outs[3] = {};
    float trigLight;
    
    bool orState = false;
    bool trigState = false;
    const float lightLambda = 0.075;

    HCVTriggerGenerator trigger;

	LogicCombine()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 8; i++)
        {
            configInput(IN1_INPUT + i, "Gate " + std::to_string(i + 1));
        }
        configInput(POLY_INPUT, "Poly");

        configOutput(OR_OUTPUT, "OR");
        configOutput(NOR_OUTPUT, "NOR");
        configOutput(TRIG_OUTPUT, "All Triggers");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void LogicCombine::process(const ProcessArgs &args)
{
    orState = false;
    trigState = false;

    //inputs[POLY_INPUT].setChannels(16);

    for(int i = 0; i < NUM_INPUTS; i++)
    {
        ins[i] = (inputs[IN1_INPUT + i].getVoltage() >= 1.0f);

        trigs[i] = inTrigs[i].process(ins[i] ? HCV_GATE_MAG : 0.0f);

        orState = orState || ins[i];
        trigState = trigState || trigs[i];
    }

    for(int i = 0; i < inputs[POLY_INPUT].getChannels(); i++)
    {
        int polyIndex = i+8; //offset by 8, since we process the 8 non-poly channels above
        ins[polyIndex] =  inputs[POLY_INPUT].getVoltage(i) >= 1.0f;
        trigs[polyIndex] = inTrigs[polyIndex].process(ins[polyIndex] ? HCV_GATE_MAG : 0.0f);

        orState = orState || ins[polyIndex];
        trigState = trigState || trigs[polyIndex];
    }

    outs[0] = orState ? HCV_GATE_MAG : 0.0f;
    outs[1] = HCV_GATE_MAG - outs[0];
    outs[2] = triggerProcessor.process(trigState) ? HCV_GATE_MAG : 0.0f;

    outputs[OR_OUTPUT].setVoltage(outs[0]);
    outputs[NOR_OUTPUT].setVoltage(outs[1]);
    outputs[TRIG_OUTPUT].setVoltage(outs[2]);

    lights[OR_LIGHT].setBrightness(outs[0]);
    lights[NOR_LIGHT].setBrightness(outs[1]);
    lights[TRIG_LIGHT].setBrightnessSmooth(outs[2], args.sampleTime * 4);
}

struct LogicCombineWidget : HCVModuleWidget { LogicCombineWidget(LogicCombine *module); };

LogicCombineWidget::LogicCombineWidget(LogicCombine *module)
{
    setSkinPath("res/LogicCombiner.svg");
    initializeWidget(module);

    //////PARAMS//////

    //////INPUTS//////
    const int inSpacing = 40;
    const int outPos = 67;
    const int lightPos = outPos + 29;

    for(int i = 0; i < 8; i++)
    {
        addInput(createInput<PJ301MPort>(Vec(10, 50 + (i*inSpacing)), module, LogicCombine::IN1_INPUT + i));
    }

    addInput(createInput<PJ301MPort>(Vec(outPos, 50), module, LogicCombine::POLY_INPUT));

    //////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(outPos, 150), module, LogicCombine::OR_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(outPos, 195), module, LogicCombine::NOR_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(outPos, 240), module, LogicCombine::TRIG_OUTPUT));

    //////BLINKENLIGHTS//////
    addChild(createLight<SmallLight<RedLight>>(Vec(lightPos, 158), module, LogicCombine::OR_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(lightPos, 203), module, LogicCombine::NOR_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(lightPos, 248), module, LogicCombine::TRIG_LIGHT));
}

Model *modelLogicCombine = createModel<LogicCombine, LogicCombineWidget>("LogicCombine");
