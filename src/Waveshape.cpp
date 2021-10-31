#include "HetrickCV.hpp"

/*                                                   
                        ┌──────────────┐             
                     ┌──┘  waveshaper  │             
                  ┌──┘                 │             
               ┌──┘                    │             
            ┌──┘                       │             
        ┌───┘                          │             
    ┌───┘                              │             
 ───┘                                  └─────────────
*/                                                   

struct Waveshape : HCVModule
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

	Waveshape()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(Waveshape::AMOUNT_PARAM, -5.0, 5.0, 0.0, "Waveshape Amount");
		configParam(Waveshape::SCALE_PARAM, -1.0, 1.0, 1.0, "Waveshape CV Depth");
		configSwitch(Waveshape::RANGE_PARAM, 0.0, 1.0, 0.0, "Input Voltage Range", {"5V", "10V"});

		configInput(AMOUNT_INPUT, "Waveshape CV");
		configInput(MAIN_INPUT, "Main");
		configOutput(MAIN_OUTPUT, "Main");
	}

	void process(const ProcessArgs &args) override;

	template <typename T = float>
	T hyperbolicWaveshaper(T _input, T _shape)
	{
		const T shapeB = (1.0 - _shape) / (1.0 + _shape);
		const T shapeA = (4.0 * _shape) / ((1.0 - _shape) * (1.0 + _shape));

		T output = _input * (shapeA + shapeB);
		output = output / ((abs(_input) * shapeA) + shapeB);

		return output;
	}

	float upscale = 5.0f;
	float downscale = 0.2f;
};


void Waveshape::process(const ProcessArgs &args)
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

	int channels = std::max(1, inputs[MAIN_INPUT].getChannels());
	simd::float_4 ins[4], shapes[4];

	for (int c = 0; c < channels; c += 4) 
	{
		ins[c / 4] = simd::float_4::load(inputs[MAIN_INPUT].getVoltages(c));
		shapes[c / 4] = simd::float_4::load(inputs[AMOUNT_INPUT].getVoltages(c));
		shapes[c / 4] = (shapes[c / 4] * scale) + amount;

		ins[c / 4] = clamp(ins[c / 4], -upscale, upscale) * downscale;

		shapes[c / 4] = clamp(shapes[c / 4], -5.0f, 5.0f) * 0.2f;
		shapes[c / 4] *= 0.99f;

		ins[c / 4] = hyperbolicWaveshaper(ins[c / 4], shapes[c / 4]);
		ins[c / 4] *= upscale;
	}

	outputs[MAIN_OUTPUT].setChannels(channels);
	for (int c = 0; c < channels; c += 4) 
	{
		ins[c / 4].store(outputs[MAIN_OUTPUT].getVoltages(c));
	}
}


struct WaveshapeWidget : HCVModuleWidget { WaveshapeWidget(Waveshape *module); };

WaveshapeWidget::WaveshapeWidget(Waveshape *module)
{
	setSkinPath("res/Waveshape.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Waveshape::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Waveshape::SCALE_PARAM);
	createHCVSwitchHoriz(35, 200, Waveshape::RANGE_PARAM);

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 235), module, Waveshape::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Waveshape::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Waveshape::MAIN_OUTPUT));
}

Model *modelWaveshape = createModel<Waveshape, WaveshapeWidget>("Waveshaper");
