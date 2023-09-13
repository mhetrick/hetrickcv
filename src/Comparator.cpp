#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h"

struct Comparator : HCVModule
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

	Comparator()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Comparator::AMOUNT_PARAM, -5.0, 5.0, 0.0, "Compare Threshold");
		configParam(Comparator::SCALE_PARAM, -1.0, 1.0, 1.0, "Compare CV Depth");

		configInput(AMOUNT_INPUT, "Compare CV");

		configOutput(GT_GATE_OUTPUT, "Greater Than Gate");
		configOutput(GT_TRIG_OUTPUT, "Greater Than Trigger");
		configOutput(LT_GATE_OUTPUT, "Less Than Gate");
		configOutput(LT_TRIG_OUTPUT, "Less Than Trigger");
		configOutput(ZEROX_OUTPUT, "Crossing Trigger");
	}

	HCVTriggeredGate ltTrig, gtTrig;

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Comparator::process(const ProcessArgs &args)
{
	float input = inputs[MAIN_INPUT].getVoltage();

	float compare = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
	compare = clamp(compare, -5.0f, 5.0f);

	const bool greaterThan = (input > compare);
	const bool lessThan = (input < compare);

	outputs[GT_TRIG_OUTPUT].setVoltage(gtTrig.process(greaterThan) ? HCV_GATE_MAG : 0.0f);
	outputs[LT_TRIG_OUTPUT].setVoltage(ltTrig.process(lessThan) ? HCV_GATE_MAG : 0.0f);
	outputs[GT_GATE_OUTPUT].setVoltage(greaterThan ? HCV_GATE_MAG : 0.0f);
	outputs[LT_GATE_OUTPUT].setVoltage(lessThan ? HCV_GATE_MAG : 0.0f);

	float allTrigs = outputs[GT_TRIG_OUTPUT].getVoltage() + outputs[LT_TRIG_OUTPUT].getVoltage();
	allTrigs = clamp(allTrigs, 0.0f, HCV_GATE_MAG);

	outputs[ZEROX_OUTPUT].setVoltage(allTrigs);

	setLightSmoothFromOutput(GT_LIGHT, GT_GATE_OUTPUT);
	setLightSmoothFromOutput(LT_LIGHT, LT_GATE_OUTPUT);
	lights[ZEROX_LIGHT].setSmoothBrightness(allTrigs, 10);
}


struct ComparatorWidget : HCVModuleWidget { ComparatorWidget(Comparator *module); };

ComparatorWidget::ComparatorWidget(Comparator* module)
{
	setSkinPath("res/Comparator.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Comparator::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Comparator::SCALE_PARAM);

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 195), module, Comparator::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Comparator::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(12, 285), module, Comparator::LT_GATE_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(53, 285), module, Comparator::GT_GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(12, 315), module, Comparator::LT_TRIG_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(53, 315), module, Comparator::GT_TRIG_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(32.5, 245), module, Comparator::ZEROX_OUTPUT));

	//////BLINKENLIGHTS//////
	addChild(createLight<SmallLight<RedLight>>(Vec(22, 275), module, Comparator::LT_LIGHT));
    addChild(createLight<SmallLight<GreenLight>>(Vec(62, 275), module, Comparator::GT_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(42, 275), module, Comparator::ZEROX_LIGHT));
}

Model *modelComparator = createModel<Comparator, ComparatorWidget>("Comparator");
