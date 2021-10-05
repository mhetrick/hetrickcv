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
	const bool mode5V = (params[RANGE_PARAM].getValue() == 0.0f);
	const float amount = params[AMOUNT_PARAM].getValue();
	const float scale = params[SCALE_PARAM].getValue();

	int channels = std::max(1, inputs[MAIN_INPUT].getChannels());
	for (int c = 0; c < channels; c ++)
	{
		float input = inputs[MAIN_INPUT].getPolyVoltage(c);

		if(mode5V) input = clamp(input, -5.0f, 5.0f) * 0.2f;
		else input = clamp(input, -10.0f, 10.0f) * 0.1f;

		float shape = amount + (inputs[AMOUNT_INPUT].getPolyVoltage(c) * scale);
		shape = clamp(shape, -5.0f, 5.0f) * 0.2f;
		shape *= 0.99f;

		const float shapeB = (1.0 - shape) / (1.0 + shape);
		const float shapeA = (4.0 * shape) / ((1.0 - shape) * (1.0 + shape));

		float output = input * (shapeA + shapeB);
		output = output / ((std::abs(input) * shapeA) + shapeB);

		if(mode5V) output *= 5.0f;
		else output *= 10.0f;

		outputs[MAIN_OUTPUT].setVoltage(output, c);
	}
	outputs[MAIN_OUTPUT].setChannels(channels);
}


struct WaveshapeWidget : HCVModuleWidget { WaveshapeWidget(Waveshape *module); };

WaveshapeWidget::WaveshapeWidget(Waveshape *module)
{
	setSkinPath("res/Waveshape.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Waveshape::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Waveshape::SCALE_PARAM);
    addParam(createParam<CKSSRot>(Vec(35, 200), module, Waveshape::RANGE_PARAM));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 235), module, Waveshape::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Waveshape::AMOUNT_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Waveshape::MAIN_OUTPUT));
}

Model *modelWaveshape = createModel<Waveshape, WaveshapeWidget>("Waveshaper");
