#include "HetrickCV.hpp"

struct Exponent : HCVModule
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

	Exponent()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(Exponent::AMOUNT_PARAM, -5.0, 5.0, 0.0, "Shape");
		configParam(Exponent::SCALE_PARAM, -1.0, 1.0, 1.0, "Shape CV Depth");
		configSwitch(Exponent::RANGE_PARAM, 0.0, 1.0, 0.0, "Input Voltage Range", {"5V", "10V"});

		configInput(AMOUNT_INPUT, "Shape CV");
		configInput(MAIN_INPUT, "Main");
		configOutput(MAIN_OUTPUT, "Main");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Exponent::process(const ProcessArgs &args)
{
	float input = inputs[MAIN_INPUT].getVoltage();
	const bool negativeInput = input < 0.0f;

    bool mode5V = (params[RANGE_PARAM].getValue() == 0.0f);
    if(mode5V) input = clamp(input, -5.0f, 5.0f) * 0.2f;
	else input = clamp(input, -10.0f, 10.0f) * 0.1f;
	input = std::abs(input);

    float exponent = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
    exponent = clamp(exponent, -5.0f, 5.0f) * 0.2f;

	if(exponent < 0)
	{
		exponent = 1.0f - (exponent * -0.5f);
	}
	else exponent += 1.0f;

    float output = powf(input, exponent);

	if (negativeInput) output *= -1.0f;
    if(mode5V) output *= 5.0f;
    else output *= 10.0f;

    outputs[MAIN_OUTPUT].setVoltage(output);
}

struct ExponentWidget : HCVModuleWidget { ExponentWidget(Exponent *module); };

ExponentWidget::ExponentWidget(Exponent *module)
{
	setSkinPath("res/Exponent.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Exponent::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Exponent::SCALE_PARAM);
    addParam(createParam<CKSSRot>(Vec(35, 200), module, Exponent::RANGE_PARAM));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 235), module, Exponent::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Exponent::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Exponent::MAIN_OUTPUT));
}

Model *modelExponent = createModel<Exponent, ExponentWidget>("Exponent");
