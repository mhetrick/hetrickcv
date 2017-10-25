#include "HetrickCV.hpp"

struct LogicInvert : Module 
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
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
		NUM_OUTPUTS
	};

    float ins[6] = {};
    float outs[6] = {};

	LogicInvert() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) 
	{
		
	}

	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void LogicInvert::step() 
{
    ins[0] = (inputs[IN1_INPUT].value >= 1.0f) ? 5.0f : 0.0f;
    ins[1] = (inputs[IN2_INPUT].value >= 1.0f) ? 5.0f : 0.0f;
    ins[2] = (inputs[IN3_INPUT].value >= 1.0f) ? 5.0f : 0.0f;
    ins[3] = (inputs[IN4_INPUT].value >= 1.0f) ? 5.0f : 0.0f;
    ins[4] = (inputs[IN5_INPUT].value >= 1.0f) ? 5.0f : 0.0f;
    ins[5] = (inputs[IN6_INPUT].value >= 1.0f) ? 5.0f : 0.0f;

    outs[0] = 5.0f - ins[0];
    outs[1] = 5.0f - ins[1];
    outs[2] = 5.0f - ins[2];
    outs[3] = 5.0f - ins[3];
    outs[4] = 5.0f - ins[4];
    outs[5] = 5.0f - ins[5];

    outputs[OUT1_OUTPUT].value = outs[0];
    outputs[OUT2_OUTPUT].value = outs[1];
    outputs[OUT3_OUTPUT].value = outs[2];
    outputs[OUT4_OUTPUT].value = outs[3];
    outputs[OUT5_OUTPUT].value = outs[4];
    outputs[OUT6_OUTPUT].value = outs[5];
}


LogicInvertWidget::LogicInvertWidget() 
{
	auto *module = new LogicInvert();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/LogicInverter.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 60), module, LogicInvert::IN1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 105), module, LogicInvert::IN2_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 150), module, LogicInvert::IN3_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 195), module, LogicInvert::IN4_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 240), module, LogicInvert::IN5_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 285), module, LogicInvert::IN6_INPUT));

    //////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(45, 60), module, LogicInvert::OUT1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 105), module, LogicInvert::OUT2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 150), module, LogicInvert::OUT3_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 195), module, LogicInvert::OUT4_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 240), module, LogicInvert::OUT5_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 285), module, LogicInvert::OUT6_OUTPUT));

    //////BLINKENLIGHTS//////
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 68), &module->outs[0]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 113), &module->outs[1]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 158), &module->outs[2]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 203), &module->outs[3]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 248), &module->outs[4]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(74, 293), &module->outs[5]));
}
