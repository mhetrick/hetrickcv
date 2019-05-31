#include "HetrickCV.hpp"

struct Delta : Module
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
		CHANGE_OUTPUT,
        DELTA_OUTPUT,
		NUM_OUTPUTS
	};

	 enum LightIds
    {
        GT_LIGHT,
        LT_LIGHT,
		CHANGE_LIGHT,
        NUM_LIGHTS
	};

	Delta() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{

	}

	TriggerGenWithSchmitt ltTrig, gtTrig;
    float lastInput = 0.0f;
    bool rising = false;
    bool falling = false;

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Delta::process(const ProcessArgs &args)
{
	float input = inputs[MAIN_INPUT].getVoltage();

    float delta = input - lastInput;
    lastInput = input;

    rising = (delta > 0.0f);
    falling = (delta < 0.0f);

	float boost = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
	boost = clampf(boost, 0.0f, 5.0f) * 8000.0f + 1;

	outputs[GT_TRIG_OUTPUT].setVoltage(gtTrig.process(rising) ? 5.0f : 0.0f);
	outputs[LT_TRIG_OUTPUT].setVoltage(ltTrig.process(falling) ? 5.0f : 0.0f);
	outputs[GT_GATE_OUTPUT].setVoltage(rising ? 5.0f : 0.0f);
	outputs[LT_GATE_OUTPUT].setVoltage(falling ? 5.0f : 0.0f);

	float allTrigs = outputs[GT_TRIG_OUTPUT].value + outputs[LT_TRIG_OUTPUT].value;
	allTrigs = clampf(allTrigs, 0.0f, 5.0f);

    const float deltaOutput = clampf(delta * boost, -5.0f, 5.0f);

	outputs[CHANGE_OUTPUT].setVoltage(allTrigs);
    outputs[DELTA_OUTPUT].setVoltage(deltaOutput);

	lights[GT_LIGHT].setBrightnessSmooth(outputs[GT_GATE_OUTPUT].value);
	lights[LT_LIGHT].setBrightnessSmooth(outputs[LT_GATE_OUTPUT].value);
	lights[CHANGE_LIGHT].setBrightnessSmooth(allTrigs);
}


struct DeltaWidget : ModuleWidget { DeltaWidget(Delta *module); };

DeltaWidget::DeltaWidget(Delta *module) : ModuleWidget(module)
{
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Delta.svg")));
		addChild(panel);
	}

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

	//////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(27, 62), module, Delta::AMOUNT_PARAM, 0.0, 5.0, 0.0));
    addParam(createParam<Trimpot>(Vec(36, 112), module, Delta::SCALE_PARAM, -1.0, 1.0, 1.0));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(12, 195), module, Delta::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Delta::AMOUNT_INPUT));

	//////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(53, 195), module, Delta::DELTA_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(12, 285), module, Delta::LT_GATE_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(53, 285), module, Delta::GT_GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(12, 315), module, Delta::LT_TRIG_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(53, 315), module, Delta::GT_TRIG_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(32.5, 245), module, Delta::CHANGE_OUTPUT));

	//////BLINKENLIGHTS//////
	addChild(createLight<SmallLight<RedLight>>(Vec(22, 275), module, Delta::LT_LIGHT));
    addChild(createLight<SmallLight<GreenLight>>(Vec(62, 275), module, Delta::GT_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(42, 275), module, Delta::CHANGE_LIGHT));
}

Model *modelDelta = createModel<Delta, DeltaWidget>("Delta");
