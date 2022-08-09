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
		const T shapeB = (1.0f - _shape) / (1.0f + _shape);
		const T shapeA = (4.0f * _shape) / ((1.0f - _shape) * (1.0f + _shape));

		T output = _input * (shapeA + shapeB);
		output = output / ((abs(_input) * shapeA) + shapeB);

		return output;
	}

	simd::float_4 ins[4] = {0.0f, 0.0f, 0.0f, 0.0f}, shapes[4] = {0.0f, 0.0f, 0.0f, 0.0f};

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

	int channels = getMaxInputPolyphony();
	outputs[MAIN_OUTPUT].setChannels(channels);

	for (int c = 0; c < channels; c += 4) 
	{
		const int vectorIndex = c/4;
		ins[vectorIndex] = simd::float_4::load(inputs[MAIN_INPUT].getVoltages(c));
		shapes[vectorIndex] = simd::float_4::load(inputs[AMOUNT_INPUT].getVoltages(c));
		shapes[vectorIndex] = (shapes[vectorIndex] * scale) + amount;

		ins[vectorIndex] = clamp(ins[vectorIndex], -upscale, upscale) * downscale;

		shapes[vectorIndex] = clamp(shapes[vectorIndex], -5.0f, 5.0f) * 0.2f;
		shapes[vectorIndex] *= 0.99f;

		ins[vectorIndex] = hyperbolicWaveshaper(ins[vectorIndex], shapes[vectorIndex]);
		ins[vectorIndex] *= upscale;

		ins[vectorIndex].store(outputs[MAIN_OUTPUT].getVoltages(c));
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
