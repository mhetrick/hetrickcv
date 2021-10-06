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
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Bitshift::process(const ProcessArgs &args)
{
	float input = inputs[MAIN_INPUT].getVoltage();

    bool mode5V = (params[RANGE_PARAM].getValue() == 0.0f);
    if(mode5V) input = clamp(input, -5.0f, 5.0f) * 0.2f;
	else input = clamp(input, -10.0f, 10.0f) * 0.1f;

	float shift = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
	shift = clamp(shift, -5.0f, 5.0f) * 0.2f;
	shift *= 31.0f;

	int finalShift = round(shift);
	int intInput = round(input * 2147483647.0f);
	int shiftedInput;

	if(finalShift > 0) shiftedInput = intInput >> finalShift;
	else
	{
		finalShift *= -1;
		shiftedInput = intInput << finalShift;
	}

	float output = shiftedInput/2147483647.0f;
	output = clamp(output, -1.0f, 1.0f);

    if(mode5V) output *= 5.0f;
    else output *= 10.0f;

    outputs[MAIN_OUTPUT].setVoltage(output);
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
