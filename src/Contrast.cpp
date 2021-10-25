#include "HetrickCV.hpp"

/*                                      
 ┌─────────────────┬──────────────────┐ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│◑
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥contrast▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 │■■■■■■■■■■■■■■■■■│▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥▥│ 
 └─────────────────┴──────────────────┘ 
*/                                      

struct Contrast : HCVModule
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

	Contrast()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(Contrast::AMOUNT_PARAM, 0, 5.0, 0.0, "Contrast");
		configParam(Contrast::SCALE_PARAM, -1.0, 1.0, 1.0, "Contrast CV Depth");
		configSwitch(Contrast::RANGE_PARAM, 0.0, 1.0, 0.0, "Input Voltage Range", {"5V", "10V"});

		configInput(AMOUNT_INPUT, "Contrast CV");
		configInput(MAIN_INPUT, "Main");
		configOutput(MAIN_OUTPUT, "Main");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Contrast::process(const ProcessArgs &args)
{
    float input = inputs[MAIN_INPUT].getVoltage();

    bool mode5V = (params[RANGE_PARAM].getValue() == 0.0f);
    if(mode5V) input = clamp(input, -5.0f, 5.0f) * 0.2f;
    else input = clamp(input, -10.0f, 10.0f) * 0.1f;

    float contrast = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
    contrast = clamp(contrast, 0.0f, 5.0f) * 0.2f;

    const float factor1 = input * 1.57143;
    const float factor2 = sinf(input * 6.28571) * contrast;

    float output = sinf(factor1 + factor2);

    if(mode5V) output *= 5.0f;
    else output *= 10.0f;

    outputs[MAIN_OUTPUT].setVoltage(output);
}


struct ContrastWidget : HCVModuleWidget { ContrastWidget(Contrast *module); };

ContrastWidget::ContrastWidget(Contrast *module)
{
	setSkinPath("res/Contrast.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Contrast::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Contrast::SCALE_PARAM);
    addParam(createParam<CKSSRot>(Vec(35, 200), module, Contrast::RANGE_PARAM));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 235), module, Contrast::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Contrast::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Contrast::MAIN_OUTPUT));
}

Model *modelContrast = createModel<Contrast, ContrastWidget>("Contrast");
