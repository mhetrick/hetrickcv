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

	BinaryNoise()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
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

    float lastOut = 0.0f;
    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slew;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void BinaryNoise::process(const ProcessArgs &args)
{
   double sr = getSampleRateParameter(SRATE_PARAM, SRATE_INPUT, SRATE_SCALE_PARAM, RANGE_PARAM);
   sRate.setSampleRateFactor(sr);

   bool isReady = sRate.readyForNextSample();
   if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());

   if(isReady)
   {
       bipolar = params[POLARITY_PARAM].getValue() == 0.0f;

       float prob = getNormalizedModulatedValue(PROB_PARAM, PROB_INPUT, PROB_SCALE_PARAM);
       bool on = random::uniform() < prob;
       lastOut = on ? 5.0f : (bipolar ? -5.0f : 0.0f);

       slew.setTargetValue(lastOut);
   }

   if(params[SLEW_PARAM].getValue() == 1.0f)
   {
       slew.setSRFactor(sRate.getSampleRateFactor());
       lastOut = slew();
   }

   outputs[MAIN_OUTPUT].setVoltage(lastOut);
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
}

Model *modelBinaryNoise = createModel<BinaryNoise, BinaryNoiseWidget>("BinaryNoise");
