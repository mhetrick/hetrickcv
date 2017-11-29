#include "HetrickCV.hpp"

struct Comparator : Module 
{
	enum ParamIds 
	{
		AMOUNT_PARAM,
        SCALE_PARAM,
		NUM_PARAMS
	};
	enum InputIds 
	{
        MAIN_INPUT,
        AMOUNT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds 
	{
		GT_GATE_OUTPUT,
		GT_TRIG_OUTPUT,
		LT_GATE_OUTPUT,
		LT_TRIG_OUTPUT,
		ZEROX_OUTPUT,
		NUM_OUTPUTS
	};

	 enum LightIds 
    {
        GT_LIGHT,
        LT_LIGHT,
		ZEROX_LIGHT,
        NUM_LIGHTS
	};

	Comparator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) 
	{
		
	}

	TriggerGenWithSchmitt ltTrig, gtTrig;

	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Comparator::step() 
{
	float input = inputs[MAIN_INPUT].value;

	float compare = params[AMOUNT_PARAM].value + (inputs[AMOUNT_INPUT].value * params[SCALE_PARAM].value);
	compare = clampf(compare, -5.0f, 5.0f);

	const bool greaterThan = (input > compare);
	
	outputs[GT_TRIG_OUTPUT].value = gtTrig.process(greaterThan) ? 5.0f : 0.0f;
	outputs[LT_TRIG_OUTPUT].value = ltTrig.process(!greaterThan) ? 5.0f : 0.0f;
	outputs[GT_GATE_OUTPUT].value = greaterThan ? 5.0f : 0.0f;
	outputs[LT_GATE_OUTPUT].value = !greaterThan ? 5.0f : 0.0f;

	float allTrigs = outputs[GT_TRIG_OUTPUT].value + outputs[LT_TRIG_OUTPUT].value;
	allTrigs = clampf(allTrigs, 0.0f, 5.0f);

	outputs[ZEROX_OUTPUT].value = allTrigs;
	
	lights[GT_LIGHT].setBrightnessSmooth(outputs[GT_GATE_OUTPUT].value);
	lights[LT_LIGHT].setBrightnessSmooth(outputs[LT_GATE_OUTPUT].value);
	lights[ZEROX_LIGHT].setBrightnessSmooth(allTrigs);
}


ComparatorWidget::ComparatorWidget() 
{
	auto *module = new Comparator();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Comparator.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	//////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(27, 62), module, Comparator::AMOUNT_PARAM, -5.0, 5.0, 0.0));
    addParam(createParam<Trimpot>(Vec(36, 112), module, Comparator::SCALE_PARAM, -1.0, 1.0, 1.0));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 195), module, Comparator::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Comparator::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(10, 285), module, Comparator::LT_GATE_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(55, 285), module, Comparator::GT_GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(10, 315), module, Comparator::LT_TRIG_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(55, 315), module, Comparator::GT_TRIG_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(32.5, 245), module, Comparator::ZEROX_OUTPUT));

	//////BLINKENLIGHTS//////
	addChild(createLight<SmallLight<RedLight>>(Vec(19, 275), module, Comparator::LT_LIGHT));
    addChild(createLight<SmallLight<GreenLight>>(Vec(64, 275), module, Comparator::GT_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(41.5, 275), module, Comparator::ZEROX_LIGHT));
}
