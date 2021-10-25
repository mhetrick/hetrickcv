#include "HetrickCV.hpp"

struct Dust : HCVModule
{
	enum ParamIds
	{
		RATE_PARAM,
		BIPOLAR_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
		RATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		DUST_OUTPUT,
		NUM_OUTPUTS
	};

	float lastDensity = 0.0;
	float densityScaled = 0.0;
	float threshold = 0.0;

	Dust()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(Dust::RATE_PARAM, 0, 4.0, 0.0, "Rate");
		configSwitch(Dust::BIPOLAR_PARAM, 0.0, 1.0, 0.0, "Output Mode", {"Bipolar", "Unipolar"});

		configInput(RATE_INPUT, "Rate CV");
		configOutput(DUST_OUTPUT, "Dust");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Dust::process(const ProcessArgs &args)
{
	float densityInput = params[RATE_PARAM].getValue() + inputs[RATE_INPUT].getVoltage();

	if(lastDensity != densityInput)
	{
		densityScaled = clamp(densityInput, 0.0f, 4.0f) / 4.0f;
		densityScaled = args.sampleRate * powf(densityScaled, 3.0f);
		lastDensity = densityInput;
		threshold = (1.0/args.sampleRate) * densityScaled;
	}

	const float noiseValue = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

	if (noiseValue < threshold)
	{
		const bool bipolar = (params[BIPOLAR_PARAM].getValue() == 0.0);

		if(bipolar)
		{
			const float scale = (threshold > 0.0f) ? 2.0f/threshold : 0.0f;
			outputs[DUST_OUTPUT].setVoltage(clamp((noiseValue * scale - 1.0f) * 5.0f, -5.0f, 5.0f));
		}
		else
		{
			const float scale = (threshold > 0.0f) ? 1.0f/threshold : 0.0f;
			outputs[DUST_OUTPUT].setVoltage(clamp(noiseValue * scale * 5.0f, 0.0f, 5.0f));
		}
	}
	else
	{
		outputs[DUST_OUTPUT].setVoltage(0.0);
	}
}

struct DustWidget : HCVModuleWidget { DustWidget(Dust *module); };

DustWidget::DustWidget(Dust *module)
{
	setSkinPath("res/Dust.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(28, 87, Dust::RATE_PARAM);
	addParam(createParam<CKSS>(Vec(37, 220), module, Dust::BIPOLAR_PARAM));

	//////INPUTS//////
	createInputPort(33, 146, Dust::RATE_INPUT);

	//////OUTPUTS//////
	createOutputPort(33, 285, Dust::DUST_OUTPUT);
}

Model *modelDust = createModel<Dust, DustWidget>("Dust");
