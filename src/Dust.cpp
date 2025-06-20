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

    // Arrays for polyphonic support
    float lastDensity[16] = {};
    float densityScaled[16] = {};
    float threshold[16] = {};

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
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();
	
	const bool bipolar = (params[BIPOLAR_PARAM].getValue() == 0.0);

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        float densityInput = params[RATE_PARAM].getValue() + inputs[RATE_INPUT].getPolyVoltage(c);

        if(lastDensity[c] != densityInput)
        {
            densityScaled[c] = clamp(densityInput, 0.0f, 4.0f) / 4.0f;
            densityScaled[c] = args.sampleRate * powf(densityScaled[c], 3.0f);
            lastDensity[c] = densityInput;
            threshold[c] = (1.0/args.sampleRate) * densityScaled[c];
        }

        const float noiseValue = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

        if (noiseValue < threshold[c])
        {
            if(bipolar)
            {
                const float scale = (threshold[c] > 0.0f) ? 2.0f/threshold[c] : 0.0f;
                outputs[DUST_OUTPUT].setVoltage(clamp((noiseValue * scale - 1.0f) * 5.0f, -5.0f, 5.0f), c);
            }
            else
            {
                const float scale = (threshold[c] > 0.0f) ? 1.0f/threshold[c] : 0.0f;
                outputs[DUST_OUTPUT].setVoltage(clamp(noiseValue * scale * HCV_GATE_MAG, 0.0f, HCV_GATE_MAG), c);
            }
        }
        else
        {
            outputs[DUST_OUTPUT].setVoltage(0.0f, c);
        }
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
