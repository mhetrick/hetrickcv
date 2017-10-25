#include "HetrickCV.hpp"
#include "dsp/digital.hpp"

struct LogicCombine : Module 
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
		NUM_INPUTS
	};
	enum OutputIds 
	{
        OR_OUTPUT,
        NOR_OUTPUT,
        TRIG_OUTPUT,
		NUM_OUTPUTS
	};

    bool ins[6] = {};
    bool trigs[6] = {};
    float outs[3] = {};
    float trigLight;
    SchmittTrigger inTrigs[6];
    bool orState = false;
    bool trigState = false;
    const float lightLambda = 0.075;

	LogicCombine() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) 
	{
		
	}

	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void LogicCombine::step() 
{
    ins[0] = (inputs[IN1_INPUT].value >= 1.0f);
    ins[1] = (inputs[IN2_INPUT].value >= 1.0f);
    ins[2] = (inputs[IN3_INPUT].value >= 1.0f);
    ins[3] = (inputs[IN4_INPUT].value >= 1.0f);
    ins[4] = (inputs[IN5_INPUT].value >= 1.0f);
    ins[5] = (inputs[IN6_INPUT].value >= 1.0f);

    orState = (ins[0] || ins[1] || ins[2] || ins[3] || ins[4] || ins[5]);

    trigs[0] = inTrigs[0].process(inputs[IN1_INPUT].value);
    trigs[1] = inTrigs[1].process(inputs[IN2_INPUT].value);
    trigs[2] = inTrigs[2].process(inputs[IN3_INPUT].value);
    trigs[3] = inTrigs[3].process(inputs[IN4_INPUT].value);
    trigs[4] = inTrigs[4].process(inputs[IN5_INPUT].value);
    trigs[5] = inTrigs[5].process(inputs[IN6_INPUT].value);

    trigState = (trigs[0] || trigs[1] || trigs[2] || trigs[3] || trigs[4] || trigs[5]);

    outs[0] = orState ? 5.0f : 0.0f;
    outs[1] = 5.0f - outs[0];

    if(trigState)
    {
        trigLight = 5.0f;
        outs[2] = 5.0f;
    }
    else
    {
        outs[2] = 0.0f;
    }

    outs[2] = trigState ? 5.0f : 0.0f;

    if (trigLight > 0.01)
        trigLight -= trigLight / lightLambda / engineGetSampleRate();

    outputs[OR_OUTPUT].value = outs[0];
    outputs[NOR_OUTPUT].value = outs[1];
    outputs[TRIG_OUTPUT].value = outs[2];
}


LogicCombineWidget::LogicCombineWidget() 
{
	auto *module = new LogicCombine();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/LogicCombiner.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 60), module, LogicCombine::IN1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 105), module, LogicCombine::IN2_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 150), module, LogicCombine::IN3_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 195), module, LogicCombine::IN4_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 240), module, LogicCombine::IN5_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 285), module, LogicCombine::IN6_INPUT));

    //////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(45, 150), module, LogicCombine::OR_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 195), module, LogicCombine::NOR_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 240), module, LogicCombine::TRIG_OUTPUT));

    //////BLINKENLIGHTS//////
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 158), &module->outs[0]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 203), &module->outs[1]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 248), &module->trigLight));
}
