#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"

struct Gingerbread : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM,
        SRATE_SCALE_PARAM,
        FB_PARAM,
        FB_SCALE_PARAM,
        RANGE_PARAM,
        SLEW_PARAM,
        DC_PARAM,
        RESEED_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        RESEED_INPUT,
        SRATE_INPUT,
        FB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};

	Gingerbread()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(Gingerbread::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(Gingerbread::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(Gingerbread::FB_PARAM, -5.0, 5.0, -5.0, "Feedback FM");
		configParam(Gingerbread::FB_SCALE_PARAM, -1.0, 1.0, 1.0, "Feedback FM CV Depth");

        configSwitch(Gingerbread::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(Gingerbread::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(Gingerbread::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});

        configButton(Gingerbread::RESEED_PARAM, "Reseed Button");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESEED_INPUT, "Reseed");
        configInput(SRATE_INPUT, "Sample Rate CV");
        configInput(FB_INPUT, "Feedback FM CV");

        configOutput(MAIN_OUTPUT, "Chaos");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float lastOut = 0.0f;
    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger, reseedTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slew;
    HCVGingerbreadMap gingerbread;
    HCVDCFilter dcFilter;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Gingerbread::process(const ProcessArgs &args)
{
    float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getVoltage() * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
    sr = clamp(sr, 0.01f, 1.0f);

    double feedbackCV = getNormalizedModulatedValue(FB_PARAM, FB_INPUT, FB_SCALE_PARAM);
    double feedback = feedbackCV * lastOut * 0.1f;
    sr = clamp(sr + feedback, 0.01f, 1.0f);

    float finalSr = sr*sr*sr;
    finalSr = clamp(finalSr, 0.0f, 1.0f);

    if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
    sRate.setSampleRateFactor(finalSr);

    bool isReady = sRate.readyForNextSample();
    if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());

    if(reseedTrigger.process(inputs[RESEED_INPUT].getVoltage() + params[RESEED_PARAM].getValue()))
    {
        gingerbread.reset();
        sRate.reset();
    }

    if(isReady)
    {
        lastOut = gingerbread.generate();
        slew.setTargetValue(lastOut);
    }

    if(params[SLEW_PARAM].getValue() == 1.0f)
    {
        slew.setSRFactor(sRate.getSampleRateFactor());
        lastOut = slew();
    }

    float filteredOut = lastOut;
    dcFilter.setFader(params[DC_PARAM].getValue());
    filteredOut = dcFilter.process(filteredOut);

    outputs[MAIN_OUTPUT].setVoltage(filteredOut);
}


struct GingerbreadWidget : HCVModuleWidget { GingerbreadWidget(Gingerbread *module); };

GingerbreadWidget::GingerbreadWidget(Gingerbread *module)
{
	setSkinPath("res/Gingerbread.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float trimpotY = knobY + 58.0f;
    const float paramJackY = knobY + 108.0f;

    const float rightOffset = 75.0f;
    const float knobX = 20.0f;
    const float trimpotX = knobX + 9.0f;
    const float paramJackX = knobX + 6.0f;
    

	createHCVKnob(knobX, knobY, Gingerbread::SRATE_PARAM);
	createHCVTrimpot(trimpotX, trimpotY, Gingerbread::SRATE_SCALE_PARAM);
    createInputPort(paramJackX, paramJackY, Gingerbread::SRATE_INPUT);

    createHCVKnob(knobX + rightOffset, knobY, Gingerbread::FB_PARAM);
	createHCVTrimpot(trimpotX + rightOffset, trimpotY, Gingerbread::FB_SCALE_PARAM);
    createInputPort(paramJackX + rightOffset, paramJackY, Gingerbread::FB_INPUT);

    const float switchY = 240.0f;
    createHCVSwitchVert(19.0f, switchY, Gingerbread::RANGE_PARAM);
    createHCVSwitchVert(67.0f, switchY, Gingerbread::SLEW_PARAM);
    createHCVSwitchVert(115.0f, switchY, Gingerbread::DC_PARAM);


    const float jackY = 312.0f;
	//////INPUTS//////
    createInputPort(15.0f, jackY, Gingerbread::CLOCK_INPUT);
    createInputPort(62.0f, jackY, Gingerbread::RESEED_INPUT);
    createHCVButtonSmall(66.0f, jackY - 20.0f, Gingerbread::RESEED_PARAM);

	//////OUTPUTS//////
    createOutputPort(110.0f, jackY, Gingerbread::MAIN_OUTPUT);
}

Model *modelGingerbread = createModel<Gingerbread, GingerbreadWidget>("Gingerbread");
