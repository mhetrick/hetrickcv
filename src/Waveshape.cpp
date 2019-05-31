#include "HetrickCV.hpp"

struct Waveshape : Module
{
	enum ParamIds
	{
		AMOUNT_PARAM,
        SCALE_PARAM,
        RANGE_PARAM,
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
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};

	Waveshape() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
	{
		configParam(Waveshape::AMOUNT_PARAM, -5.0, 5.0, 0.0, "");
		configParam(Waveshape::SCALE_PARAM, -1.0, 1.0, 1.0, "");
		configParam(Waveshape::RANGE_PARAM, 0.0, 1.0, 0.0, "");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Waveshape::process(const ProcessArgs &args)
{
	float input = inputs[MAIN_INPUT].getVoltage();

    bool mode5V = (params[RANGE_PARAM].getValue() == 0.0f);
    if(mode5V) input = clamp(input, -5.0f, 5.0f) * 0.2f;
	else input = clamp(input, -10.0f, 10.0f) * 0.1f;

	float shape = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
	shape = clamp(shape, -5.0f, 5.0f) * 0.2f;
	shape *= 0.99f;

	const float shapeB = (1.0 - shape) / (1.0 + shape);
	const float shapeA = (4.0 * shape) / ((1.0 - shape) * (1.0 + shape));

	float output = input * (shapeA + shapeB);
	output = output / ((std::abs(input) * shapeA) + shapeB);

    if(mode5V) output *= 5.0f;
    else output *= 10.0f;

    outputs[MAIN_OUTPUT].setVoltage(output);
}

struct CKSSRot : SVGSwitch {
	CKSSRot() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_1.svg")));
	}
};


struct WaveshapeWidget : ModuleWidget { WaveshapeWidget(Waveshape *module); };

WaveshapeWidget::WaveshapeWidget(Waveshape *module) : ModuleWidget(module)
{
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Waveshape.svg")));
		addChild(panel);
	}

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

	//////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(27, 62), module, Waveshape::AMOUNT_PARAM));
    addParam(createParam<Trimpot>(Vec(36, 112), module, Waveshape::SCALE_PARAM));
    addParam(createParam<CKSSRot>(Vec(35, 200), module, Waveshape::RANGE_PARAM));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 235), module, Waveshape::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Waveshape::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Waveshape::MAIN_OUTPUT));
}

Model *modelWaveshape = createModel<Waveshape, WaveshapeWidget>("Waveshaper");
