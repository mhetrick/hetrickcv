#include "HetrickCV.hpp"

/*                                                               
 ┌──────────────────────────────────────────────────────────────┐
 │██████████████████████████████████████████████████████████████│
 │██████████████████████████████████████████████████████████████│
 │██████████8""""8██████████████████████████████████████████████│
 │██████████8████8███e█eeeee█eeeee█e███e█e██eeee█eeeee██████████│
 │██████████8eeee8ee█8███8███8█████8███8█8██8██████8████████████│
 │██████████88█████8█8e██8e██8eeee█8eee8█8e█8eee███8e███████████│
 │██████████88█████8█88██88█████88█88██8█88█88█████88███████████│
 │██████████88eeeee8█88██88██8ee88█88██8█88█88█████88███████████│
 │██████████████████████████████████████████████████████████████│
 │██████████████████████████████████████████████████████████████│
 │██████████████████████████████████████████████████████████████│
 └──────────────────────────────────────────────────────────────┘
*/                                                               

struct Bitshift : HCVModule
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

	Bitshift()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(Bitshift::AMOUNT_PARAM, -5.0, 5.0, 0.0, "Bitshift");
		configParam(Bitshift::SCALE_PARAM, -1.0, 1.0, 1.0, "Bitshift CV Scale");
		configSwitch(Bitshift::RANGE_PARAM, 0.0, 1.0, 0.0, "Input Voltage Range", {"5V", "10V"});

		configInput(AMOUNT_INPUT, "Bitshift CV");
		configInput(MAIN_INPUT, "Main");
		configOutput(MAIN_OUTPUT, "Main");
	}

	void process(const ProcessArgs &args) override;

	float upscale = 5.0f;
	float downscale = 0.2f;

	simd::float_4 ins[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
					outs[4] = {0.0f, 0.0f, 0.0f, 0.0f},
					shifts[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	simd::int32_4 intShifts[4] = {0, 0, 0, 0},
					intIns[4] = {0, 0, 0, 0},
					shiftedIns[4] = {0, 0, 0, 0};

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Bitshift::process(const ProcessArgs &args)
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

		shifts[vectorIndex] = simd::float_4::load(inputs[AMOUNT_INPUT].getVoltages(c));
		shifts[vectorIndex] = (shifts[vectorIndex] * scale) + amount;
		shifts[vectorIndex] = clamp(shifts[vectorIndex], -5.0f, 5.0f) * 0.2f * 31.0f;

		intShifts[vectorIndex] = round(shifts[vectorIndex]);
		intIns[vectorIndex] = round(ins[vectorIndex] * 2147483647.0f);

		for (int i = 0; i < 4; i++)
		{
			if (intShifts[vectorIndex][i] > 0)
			{
				shiftedIns[vectorIndex][i] = intIns[vectorIndex][i] >> intShifts[vectorIndex][i];
			}
			else
			{
				intShifts[vectorIndex][i] *= -1;
				shiftedIns[vectorIndex][i] = intIns[vectorIndex][i] << intShifts[vectorIndex][i];
			}
		}

		outs[vectorIndex] = shiftedIns[vectorIndex]/2147483647.0f;
		outs[vectorIndex] = clamp(outs[vectorIndex], -1.0f, 1.0f) * upscale;

		outs[vectorIndex].store(outputs[MAIN_OUTPUT].getVoltages(c));
	}
}


struct BitshiftWidget : HCVModuleWidget { BitshiftWidget(Bitshift *module); };

BitshiftWidget::BitshiftWidget(Bitshift *module)
{
	setSkinPath("res/Bitshift.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Bitshift::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Bitshift::SCALE_PARAM);
    addParam(createParam<CKSSRot>(Vec(35, 200), module, Bitshift::RANGE_PARAM));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 235), module, Bitshift::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Bitshift::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Bitshift::MAIN_OUTPUT));
}

Model *modelBitshift = createModel<Bitshift, BitshiftWidget>("Bitshift");
