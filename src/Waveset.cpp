#include "HetrickCV.hpp"
#include "DSP/HCVSampleRate.h"

//json for later
// {
//     "slug": "Waveset",
//     "name": "Waveset",
//     "description": "Waveset distortion and processing effect.",
//     "tags": [
//       "Distortion",
//       "Effect",
//           "Polyphonic"
//     ]
//   },

struct Waveset : HCVModule
{
	enum ParamIds
	{
		SET_GAIN_PARAM,
        SET_GAIN_SCALE_PARAM,
        PROB_PARAM,
        PROB_SCALE_PARAM,
        MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        SET_GAIN_INPUT,
        PROB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
        SET_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
	{
		MAIN_LIGHT_POS, MAIN_LIGHT_NEG,
		NUM_LIGHTS
	};

	Waveset()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Waveset::SET_GAIN_PARAM, -1.0, 1.0, 1.0, "Set Gain");
		configParam(Waveset::SET_GAIN_SCALE_PARAM, -1.0, 1.0, 0.0, "Set Gain CV Depth");

        configParam(Waveset::PROB_PARAM, 0.0, 1.0, 0.5, "Probability or Set Percent");
		configParam(Waveset::PROB_SCALE_PARAM, -1.0, 1.0, 0.0, "Probability or Set Percent CV Depth");

        configSwitch(Waveset::MODE_PARAM, 0.0, 1.0, 1.0, "Mode", {"Fixed Set", "Probability"});

        configInput(MAIN_INPUT, "Main");
        configInput(SET_GAIN_INPUT, "Sample Rate CV");
        configInput(PROB_INPUT, "Probability CV");

        configOutput(MAIN_OUTPUT, "Main");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float lastOut = 0.0f;
    rack::dsp::SchmittTrigger clockTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slew;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Waveset::process(const ProcessArgs &args)
{
    const float probKnob = params[PROB_PARAM].getValue();
    const float probDepth = params[PROB_SCALE_PARAM].getValue() * 0.2f;

    const float gainKnob = params[SET_GAIN_PARAM].getValue();
    const float gainDepth = params[SET_GAIN_SCALE_PARAM].getValue() * 0.2f;

    bool probMode = params[MODE_PARAM].getValue() > 0.0f;

    int polyChannels = getMaxInputPolyphony();
    outputs[MAIN_OUTPUT].setChannels(polyChannels);

    for (int chan = 0; chan < polyChannels; chan++)
    {
        float input = inputs[MAIN_INPUT].getPolyVoltage(chan);

        float setGain = ((inputs[SET_GAIN_INPUT].getPolyVoltage(chan)) * gainDepth) + gainKnob;
        setGain = clamp(setGain, -1.0f, 1.0f);

        float probability = ((inputs[PROB_INPUT].getPolyVoltage(chan)) * probDepth) + probKnob;
        probability = clamp(probability, 0.0f, 1.0f);

        outputs[MAIN_OUTPUT].setVoltage(input, chan);
    }

    setBipolarLightBrightness(MAIN_LIGHT_POS, outputs[MAIN_OUTPUT].getVoltage() * 0.2f);
}


struct WavesetWidget : HCVModuleWidget { WavesetWidget(Waveset *module); };

WavesetWidget::WavesetWidget(Waveset *module)
{
	setSkinPath("res/Waveset.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float trimpotY = knobY + 58.0f;
    const float paramJackY = knobY + 108.0f;

    const float rightOffset = 75.0f;
    float knobX = 20.0f;
    float trimpotX = knobX + 9.0f;
    float paramJackX = knobX + 6.0f;
    

	createHCVKnob(knobX, knobY, Waveset::SET_GAIN_PARAM);
	createHCVTrimpot(trimpotX, trimpotY, Waveset::SET_GAIN_SCALE_PARAM);
    createInputPort(paramJackX, paramJackY, Waveset::SET_GAIN_INPUT);

    createHCVKnob(knobX + rightOffset, knobY, Waveset::PROB_PARAM);
	createHCVTrimpot(trimpotX + rightOffset, trimpotY, Waveset::PROB_SCALE_PARAM);
    createInputPort(paramJackX + rightOffset, paramJackY, Waveset::PROB_INPUT);

    float switchX = 19.0f;
    float switchY = 240.0f;
    float spacing = 48.0f;
    createHCVSwitchVert(switchX, switchY, Waveset::MODE_PARAM);


    const float jackY = 312.0f;
	//////INPUTS//////
    createInputPort(23.0f, jackY, Waveset::MAIN_INPUT);

	//////OUTPUTS//////
    createOutputPort(103.0f, jackY, Waveset::MAIN_OUTPUT);
    createHCVBipolarLightForJack(103.0f, jackY, Waveset::MAIN_LIGHT_POS);
}

Model *modelWaveset = createModel<Waveset, WavesetWidget>("Waveset");
