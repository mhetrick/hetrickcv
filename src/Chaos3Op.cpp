#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVCrackle.h"

struct Chaos3Op : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM, SRATE_SCALE_PARAM,

        CHAOSA_PARAM, CHAOSA_SCALE_PARAM,
        CHAOSB_PARAM, CHAOSB_SCALE_PARAM,
        CHAOSC_PARAM, CHAOSC_SCALE_PARAM,

        RANGE_PARAM,
        SLEW_PARAM,
        DC_PARAM,
        MODE_PARAM,
        RESEED_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        RESEED_INPUT,
        SRATE_INPUT,

        CHAOSA_INPUT,
        CHAOSB_INPUT,
        CHAOSC_INPUT,

		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        ENUMS(OUT_LIGHT, 2),
        NUM_LIGHTS
	};

	Chaos3Op()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Chaos3Op::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(Chaos3Op::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(Chaos3Op::CHAOSA_PARAM, -5.0, 5.0, 5.0, "Chaos A");
		configParam(Chaos3Op::CHAOSA_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos A CV Depth");

        configParam(Chaos3Op::CHAOSB_PARAM, -5.0, 5.0, 0.0, "Chaos B");
		configParam(Chaos3Op::CHAOSB_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos B CV Depth");

        configParam(Chaos3Op::CHAOSC_PARAM, -5.0, 5.0, 0.0, "Chaos C");
		configParam(Chaos3Op::CHAOSC_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos C CV Depth");

        configSwitch(Chaos3Op::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(Chaos3Op::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(Chaos3Op::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});
        configSwitch(Chaos3Op::MODE_PARAM, 0.0, 1.0, 0.0, "Mode", {"LCC", "Quadratic"});

        configButton(Chaos3Op::RESEED_PARAM, "Reseed Button");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESEED_INPUT, "Reseed");
        configInput(SRATE_INPUT, "Sample Rate CV");

        configInput(CHAOSA_INPUT, "Chaos A CV");
        configInput(CHAOSB_INPUT, "Chaos B CV");
        configInput(CHAOSC_INPUT, "Chaos C CV");

        configOutput(MAIN_OUTPUT, "Chaos");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float lastOut[16] = {};
    float chaosAmountA[16] = {}, chaosAmountB[16] = {}, chaosAmountC[16] = {};

    // Single boolean for all channels (front-panel switch)
    bool quadraticMode = false;

    rack::dsp::SchmittTrigger clockTrigger[16], reseedTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slew[16];
    HCVDCFilterT<float> dcFilter[16];

    // Per-channel chaos generators
    HCVLCCMap lcc[16];
    HCVQuadraticMap quadratic[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu

    void renderChaos(int channel)
    {
        if (quadraticMode)
        {
            quadratic[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel]);
            quadratic[channel].generate();
            lastOut[channel] = quadratic[channel].out;
        }
        else
        {
            lcc[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel]);
            lcc[channel].generate();
            lastOut[channel] = lcc[channel].out;
        }

    }

    void resetChaos(int channel)
    {
        if (quadraticMode)
        {
            quadratic[channel].reset();
        }
        else
        {
            lcc[channel].reset();
        }
        
        sRate[channel].reset();
    }
};

void Chaos3Op::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();
    
    // Mode is global for all channels (front-panel switch)
    quadraticMode = params[MODE_PARAM].getValue();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        const float sr = getSampleRateParameter(SRATE_PARAM, SRATE_INPUT, SRATE_SCALE_PARAM, RANGE_PARAM, c);
        sRate[c].setSampleRateFactor(sr);

        bool isReady = sRate[c].readyForNextSample();
        if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger[c].process(inputs[CLOCK_INPUT].getPolyVoltage(c));

        if(reseedTrigger[c].process(inputs[RESEED_INPUT].getPolyVoltage(c) + (c == 0 ? params[RESEED_PARAM].getValue() : 0.0f)))
        {
            resetChaos(c);
        }

        if(isReady)
        {   
            chaosAmountA[c] = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM, c);
            chaosAmountB[c] = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM, c);
            chaosAmountC[c] = getNormalizedModulatedValue(CHAOSC_PARAM, CHAOSC_INPUT, CHAOSC_SCALE_PARAM, c);

            renderChaos(c);
            slew[c].setTargetValue(lastOut[c]);
        }

        if(params[SLEW_PARAM].getValue() == 1.0f)
        {
            slew[c].setSRFactor(sRate[c].getSampleRateFactor());
            lastOut[c] = slew[c]();
        }

        float filteredOut = lastOut[c];
        dcFilter[c].setFader(params[DC_PARAM].getValue());
        filteredOut = dcFilter[c].process(filteredOut);

        outputs[MAIN_OUTPUT].setVoltage(filteredOut * 5.0f, c);
    }
    
    // Light shows the state of channel 0
    setBipolarLightBrightness(OUT_LIGHT, outputs[MAIN_OUTPUT].getVoltage(0) * 0.2f);
}


struct Chaos3OpWidget : HCVModuleWidget { Chaos3OpWidget(Chaos3Op *module); };

Chaos3OpWidget::Chaos3OpWidget(Chaos3Op *module)
{
	setSkinPath("res/Chaos3Op.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 35.0f;
    const float knobX = 10.0f;
    const float spacing = 45.0f;

    createParamComboHorizontal(knobX, knobY, Chaos3Op::SRATE_PARAM, Chaos3Op::SRATE_SCALE_PARAM, Chaos3Op::SRATE_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing, Chaos3Op::CHAOSA_PARAM, Chaos3Op::CHAOSA_SCALE_PARAM, Chaos3Op::CHAOSA_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0, Chaos3Op::CHAOSB_PARAM, Chaos3Op::CHAOSB_SCALE_PARAM, Chaos3Op::CHAOSB_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*3.0, Chaos3Op::CHAOSC_PARAM, Chaos3Op::CHAOSC_SCALE_PARAM, Chaos3Op::CHAOSC_INPUT);


    const float switchY = 238.0f;
    createHCVSwitchVert(15.0f, switchY, Chaos3Op::RANGE_PARAM);
    createHCVSwitchVert(55.0f, switchY, Chaos3Op::SLEW_PARAM);
    createHCVSwitchVert(96.0f, switchY, Chaos3Op::DC_PARAM);
    createHCVSwitchVert(142.0f, switchY, Chaos3Op::MODE_PARAM);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(23.0f, jackY, Chaos3Op::CLOCK_INPUT);
    createInputPort(78.0f, jackY, Chaos3Op::RESEED_INPUT);
    createHCVButtonSmall(82.0f, jackY - 20.0f, Chaos3Op::RESEED_PARAM);

	//////OUTPUTS//////
    createOutputPort(138.0f, jackY, Chaos3Op::MAIN_OUTPUT);
    createHCVBipolarLightForJack(138.0f, jackY, Chaos3Op::OUT_LIGHT);
}

Model *modelChaos3Op = createModel<Chaos3Op, Chaos3OpWidget>("Chaos3Op");
