#include "HetrickCV.hpp"
#include "DSP/HCVSampleRate.h"

struct BinaryNoise : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM,
        SRATE_SCALE_PARAM,
        PROB_PARAM,
        PROB_SCALE_PARAM,
        RANGE_PARAM,
        SLEW_PARAM,
        POLARITY_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        SRATE_INPUT,
        PROB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
	{
		MAIN_LIGHT_POS, MAIN_LIGHT_NEG,
		NUM_LIGHTS
	};

	BinaryNoise()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(BinaryNoise::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(BinaryNoise::SRATE_SCALE_PARAM, -1.0, 1.0, 0.0, "Sample Rate CV Depth");

        configParam(BinaryNoise::PROB_PARAM, -5.0, 5.0, 0.0, "Probability");
		configParam(BinaryNoise::PROB_SCALE_PARAM, -1.0, 1.0, 0.0, "Probability CV Depth");

        configSwitch(BinaryNoise::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(BinaryNoise::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(BinaryNoise::POLARITY_PARAM, 0.0, 1.0, 0.0, "Polarity", {"Bipolar", "Unipolar"});

        configInput(CLOCK_INPUT, "Clock");
        configInput(SRATE_INPUT, "Sample Rate CV");
        configInput(PROB_INPUT, "Probability CV");

        configOutput(MAIN_OUTPUT, "Main");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float lastOut[16] = {};
    rack::dsp::SchmittTrigger clockTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slew[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void BinaryNoise::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        double sr = getSampleRateParameter(SRATE_PARAM, SRATE_INPUT, SRATE_SCALE_PARAM, RANGE_PARAM, c);
        sRate[c].setSampleRateFactor(sr);

        bool isReady = sRate[c].readyForNextSample();
        if(inputs[CLOCK_INPUT].isConnected()) 
        {
            isReady = clockTrigger[c].process(inputs[CLOCK_INPUT].getPolyVoltage(c));
        }

        if(isReady)
        {
            float prob = getNormalizedModulatedValue(PROB_PARAM, PROB_INPUT, PROB_SCALE_PARAM, c);
            bool on = random::uniform() < prob;
            float offset = (1.0f - params[POLARITY_PARAM].getValue()) * -5.0f;

            lastOut[c] = (on ? HCV_GATE_MAG + offset : offset);

            slew[c].setTargetValue(lastOut[c]);
        }   

        if(params[SLEW_PARAM].getValue() == 1.0f)
        {
            slew[c].setSRFactor(sRate[c].getSampleRateFactor());
            lastOut[c] = slew[c]();
        }

        outputs[MAIN_OUTPUT].setVoltage(lastOut[c], c);
    }

    // Light shows the state of channel 0
    setBipolarLightBrightness(MAIN_LIGHT_POS, outputs[MAIN_OUTPUT].getVoltage(0) * 0.2f);
}


struct BinaryNoiseWidget : HCVModuleWidget { BinaryNoiseWidget(BinaryNoise *module); };

BinaryNoiseWidget::BinaryNoiseWidget(BinaryNoise *module)
{
	setSkinPath("res/BinaryNoise.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float trimpotY = knobY + 58.0f;
    const float paramJackY = knobY + 108.0f;

    const float rightOffset = 75.0f;
    float knobX = 20.0f;
    float trimpotX = knobX + 9.0f;
    float paramJackX = knobX + 6.0f;
    

	createHCVKnob(knobX, knobY, BinaryNoise::SRATE_PARAM);
	createHCVTrimpot(trimpotX, trimpotY, BinaryNoise::SRATE_SCALE_PARAM);
    createInputPort(paramJackX, paramJackY, BinaryNoise::SRATE_INPUT);

    createHCVKnob(knobX + rightOffset, knobY, BinaryNoise::PROB_PARAM);
	createHCVTrimpot(trimpotX + rightOffset, trimpotY, BinaryNoise::PROB_SCALE_PARAM);
    createInputPort(paramJackX + rightOffset, paramJackY, BinaryNoise::PROB_INPUT);

    float switchX = 19.0f;
    float switchY = 240.0f;
    float spacing = 48.0f;
    createHCVSwitchVert(switchX, switchY, BinaryNoise::RANGE_PARAM);
    createHCVSwitchVert(switchX + spacing, switchY, BinaryNoise::SLEW_PARAM);
    createHCVSwitchVert(switchX + spacing*2, switchY, BinaryNoise::POLARITY_PARAM);


    const float jackY = 312.0f;
	//////INPUTS//////
    createInputPort(23.0f, jackY, BinaryNoise::CLOCK_INPUT);

	//////OUTPUTS//////
    createOutputPort(103.0f, jackY, BinaryNoise::MAIN_OUTPUT);
    createHCVBipolarLightForJack(103.0f, jackY, BinaryNoise::MAIN_LIGHT_POS);
}

Model *modelBinaryNoise = createModel<BinaryNoise, BinaryNoiseWidget>("BinaryNoise");
