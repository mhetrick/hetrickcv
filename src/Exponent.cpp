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

	float upscale = 5.0f;
	float downscale = 0.2f;

	simd::float_4 ins[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
					outs[4] = {0.0f, 0.0f, 0.0f, 0.0f},
					expos[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Exponent::process(const ProcessArgs &args)
{
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

	const float amount = params[AMOUNT_PARAM].getValue();
	const float scale = params[SCALE_PARAM].getValue();

	int channels = getMaxInputPolyphony();
	outputs[MAIN_OUTPUT].setChannels(channels);

	for (int c = 0; c < channels; c += 4) 
	{
		const int vectorIndex = c/4;
		ins[vectorIndex] = simd::float_4::load(inputs[MAIN_INPUT].getVoltages(c));
		ins[vectorIndex] = clamp(ins[vectorIndex], -upscale, upscale) * downscale;

		expos[vectorIndex] = simd::float_4::load(inputs[AMOUNT_INPUT].getVoltages(c));
		expos[vectorIndex] = (expos[vectorIndex] * scale) + amount;
		expos[vectorIndex] = clamp(expos[vectorIndex], -5.0f, 5.0f) * 0.2f;

		for (int i = 0; i < 4; i++)
		{
			if (expos[vectorIndex][i] < 0.0f)
			{
				expos[vectorIndex][i] = 1.0f - (expos[vectorIndex][i] * -0.5f);
			}
			else expos[vectorIndex][i] += 1.0f;
		}

		outs[vectorIndex] = pow(abs(ins[vectorIndex]), expos[vectorIndex]);
		outs[vectorIndex] *= sgn(ins[vectorIndex]);
		outs[vectorIndex] *= upscale;

		outs[vectorIndex].store(outputs[MAIN_OUTPUT].getVoltages(c));
	}
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
