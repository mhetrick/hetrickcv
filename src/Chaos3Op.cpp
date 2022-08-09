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

	Chaos3Op()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
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

    float lastOut = 0.0f;

    float chaosAmountA = 0.0f, chaosAmountB = 0.0f, chaosAmountC = 0.0f;

    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger, reseedTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slew;
    HCVDCFilterT<float> dcFilter;

    HCVLCCMap lcc;
    HCVQuadraticMap quadratic;

    bool quadraticMode = false;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu

    void renderChaos()
    {
        if (quadraticMode)
        {
            quadratic.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC);
            quadratic.generate();
            lastOut = quadratic.out;
        }
        else
        {
            lcc.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC);
            lcc.generate();
            lastOut = lcc.out;
        }

    }

    void resetChaos()
    {
        if (quadraticMode)
        {
            quadratic.reset();
        }
        else
        {
            lcc.reset();
        }
    }
};

void Chaos3Op::process(const ProcessArgs &args)
{
    float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getVoltage() * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
    sr = clamp(sr, 0.01f, 1.0f);
    float finalSr = sr*sr*sr;

    if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
    sRate.setSampleRateFactor(finalSr);

    bool isReady = sRate.readyForNextSample();
    if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());

    if(reseedTrigger.process(inputs[RESEED_INPUT].getVoltage() + params[RESEED_PARAM].getValue()))
    {
        resetChaos();
        sRate.reset();
    }

    if(isReady)
    {   
        chaosAmountA = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM);
        chaosAmountB = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM);
        chaosAmountC = getNormalizedModulatedValue(CHAOSC_PARAM, CHAOSC_INPUT, CHAOSC_SCALE_PARAM);

        quadraticMode = params[MODE_PARAM].getValue();

        renderChaos();
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

    outputs[MAIN_OUTPUT].setVoltage(filteredOut * 5.0f);
    
}


struct Chaos3OpWidget : HCVModuleWidget { Chaos3OpWidget(Chaos3Op *module); };

Chaos3OpWidget::Chaos3OpWidget(Chaos3Op *module)
{
	setSkinPath("res/3OpChaos.svg");
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
}

Model *modelChaos3Op = createModel<Chaos3Op, Chaos3OpWidget>("Chaos3Op");
