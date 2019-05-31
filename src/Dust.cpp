#include "HetrickCV.hpp"

struct Dust : Module
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

	Dust() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
	{
		configParam(Dust::RATE_PARAM, 0, 4.0, 0.0, "");
		configParam(Dust::BIPOLAR_PARAM, 0.0, 1.0, 0.0, "");
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
			outputs[DUST_OUTPUT].setVoltage(clamp(noiseValue * scale * 5.0f, 5.0f, 5.0f));
		}
	}
	else
	{
		outputs[DUST_OUTPUT].setVoltage(0.0);
	}
}

struct DustWidget : ModuleWidget { DustWidget(Dust *module); };

DustWidget::DustWidget(Dust *module) : ModuleWidget(module)
{
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dust.svg")));
		addChild(panel);
	}

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

	//////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, Dust::RATE_PARAM));
	addParam(createParam<CKSS>(Vec(37, 220), module, Dust::BIPOLAR_PARAM));

	//////INPUTS//////
	addInput(createInput<PJ301MPort>(Vec(33, 146), module, Dust::RATE_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Dust::DUST_OUTPUT));
}

Model *modelDust = createModel<Dust, DustWidget>("Dust");
