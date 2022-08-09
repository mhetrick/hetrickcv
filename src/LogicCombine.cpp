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

    bool ins[NUM_INPUTS] = {};
    bool trigs[NUM_INPUTS] = {};
    float outs[3] = {};
    float trigLight;
    dsp::SchmittTrigger inTrigs[NUM_INPUTS];
    bool orState = false;
    bool trigState = false;
    const float lightLambda = 0.075;

    HCVTriggerGenerator trigger;

	LogicCombine()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
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

    for(int i = 0; i < NUM_INPUTS; i++)
    {
        ins[i] = (inputs[IN1_INPUT + i].getVoltage() >= 1.0f);
        trigs[i] = inTrigs[i].process(inputs[IN1_INPUT + i].getVoltage());

        orState = orState || ins[i];
        trigState = trigState || trigs[i];
    }

    outs[0] = orState ? 5.0f : 0.0f;
    outs[1] = 5.0f - outs[0];

    if(trigState)
    {
        trigger.trigger();
        lights[TRIG_LIGHT].value = 5.0f;
    }

    outs[2] = trigger.process() ? 5.0f : 0.0f;

    if (lights[TRIG_LIGHT].value > 0.01)
        lights[TRIG_LIGHT].value -= lights[TRIG_LIGHT].value / lightLambda * args.sampleTime;

    outputs[OR_OUTPUT].setVoltage(outs[0]);
    outputs[NOR_OUTPUT].setVoltage(outs[1]);
    outputs[TRIG_OUTPUT].setVoltage(outs[2]);

    lights[OR_LIGHT].setBrightness(outs[0]);
    lights[NOR_LIGHT].setBrightness(outs[1]);
    lights[TRIG_LIGHT].setSmoothBrightness(outs[2], 10);
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

    for(int i = 0; i < LogicCombine::NUM_INPUTS; i++)
    {
        addInput(createInput<PJ301MPort>(Vec(10, 50 + (i*inSpacing)), module, LogicCombine::IN1_INPUT + i));
    }

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
