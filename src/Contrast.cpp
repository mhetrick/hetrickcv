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

	float upscale = 5.0f;
	float downscale = 0.2f;

	simd::float_4 ins[4] = {0.0f, 0.0f, 0.0f, 0.0f}, contrasts[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	template <typename T = float>
	T contrastAlgo(T _input, T _contrast)
	{
		const T factor1 = _input * 1.57143;
    	const T factor2 = sin(_input * 6.28571) * _contrast;

    	return sin(factor1 + factor2);
	}
};


void Contrast::process(const ProcessArgs &args)
{
	const float amount = params[AMOUNT_PARAM].getValue();
	const float scale = params[SCALE_PARAM].getValue();

	if (params[RANGE_PARAM].getValue() == 0.0f)
	{
		upscale = 5.0f;
		downscale = 0.2f;
	}
	else
	{
		upscale = 10.0f;
		downscale = 0.1f;
	}

	int channels = getMaxInputPolyphony();
	outputs[MAIN_OUTPUT].setChannels(channels);

	for (int c = 0; c < channels; c += 4) 
	{
		const int vectorIndex = c / 4;
		ins[vectorIndex] = simd::float_4::load(inputs[MAIN_INPUT].getVoltages(c));
		contrasts[vectorIndex] = simd::float_4::load(inputs[AMOUNT_INPUT].getVoltages(c));
		contrasts[vectorIndex] = (contrasts[vectorIndex] * scale) + amount;

		ins[vectorIndex] = clamp(ins[vectorIndex], -upscale, upscale) * downscale;

		contrasts[vectorIndex] = clamp(contrasts[vectorIndex], 0.0f, 5.0f) * 0.2f;

		ins[vectorIndex] = contrastAlgo(ins[vectorIndex], contrasts[vectorIndex]);
		ins[vectorIndex] *= upscale;

		ins[c / 4].store(outputs[MAIN_OUTPUT].getVoltages(c));
	}
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
