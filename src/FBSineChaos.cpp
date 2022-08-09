#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVCrackle.h"

struct FBSineChaos : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM, SRATE_SCALE_PARAM,

        CHAOSA_PARAM, CHAOSA_SCALE_PARAM,
        CHAOSB_PARAM, CHAOSB_SCALE_PARAM,
        CHAOSC_PARAM, CHAOSC_SCALE_PARAM,
        CHAOSD_PARAM, CHAOSD_SCALE_PARAM,

        MODE_PARAM,

        RANGE_PARAM,
        SLEW_PARAM,
        DC_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        SRATE_INPUT,

        CHAOSA_INPUT,
        CHAOSB_INPUT,
        CHAOSC_INPUT,
        CHAOSD_INPUT,
        
		NUM_INPUTS
	};
	enum OutputIds
	{
		X_OUTPUT,
        Y_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        NUM_LIGHTS = 8
	};

	FBSineChaos()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FBSineChaos::SRATE_PARAM, 0.01, 1.0, 1.0, "Sample Rate");
		configParam(FBSineChaos::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(FBSineChaos::CHAOSA_PARAM, -5.0, 5.0, -3.0, "Index Multiplier");
		configParam(FBSineChaos::CHAOSA_SCALE_PARAM, -1.0, 1.0, 1.0, "Index Multiplier CV Depth");

        configParam(FBSineChaos::CHAOSB_PARAM, -5.0, 5.0, 0.10, "Phase Increment");
		configParam(FBSineChaos::CHAOSB_SCALE_PARAM, -1.0, 1.0, 1.0, "Phase Increment CV Depth");

        configParam(FBSineChaos::CHAOSC_PARAM, -5.0, 5.0, -5.0, "Phase Multiplier");
		configParam(FBSineChaos::CHAOSC_SCALE_PARAM, -1.0, 1.0, 1.0, "Phase Multiplier CV Depth");

        configParam(FBSineChaos::CHAOSD_PARAM, -5.0, 5.0, 0.0, "Feedback");
		configParam(FBSineChaos::CHAOSD_SCALE_PARAM, -1.0, 1.0, 1.0, "Feedback CV Depth");

        configSwitch(FBSineChaos::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(FBSineChaos::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(FBSineChaos::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});
        configSwitch(FBSineChaos::MODE_PARAM, 0.0, 1.0, 0.0, "Negative Phase Mode", {"Normal", "Broken"});

        configInput(CLOCK_INPUT, "Clock");
        configInput(SRATE_INPUT, "Sample Rate CV");

        configInput(CHAOSA_INPUT, "Index Multiplier CV");
        configInput(CHAOSB_INPUT, "Phase Increment CV");
        configInput(CHAOSC_INPUT, "Phase Multiplier CV");
        configInput(CHAOSD_INPUT, "Feedback CV CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y (Phase)");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float xVal = 0.0f, yVal = 0.0f;
 
    float chaosAmountA = 0.0f, chaosAmountB = 0.0f, chaosAmountC = 0.0f, chaosAmountD = 0.0f;

    rack::dsp::SchmittTrigger clockTrigger, reseedTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slewX, slewY;
    HCVDCFilterT<simd::float_4> dcFilter;

    HCVFBSineChaos fbSine;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void FBSineChaos::process(const ProcessArgs &args)
{
    float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getVoltage() * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
    sr = clamp(sr, 0.01f, 1.0f);
    float finalSr = sr*sr*sr;

    if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
    sRate.setSampleRateFactor(finalSr);

    bool isReady = sRate.readyForNextSample();
    if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());

    if(isReady)
    {   
        chaosAmountA = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM);
        chaosAmountB = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM);
        chaosAmountC = getNormalizedModulatedValue(CHAOSC_PARAM, CHAOSC_INPUT, CHAOSC_SCALE_PARAM);
        chaosAmountD = getNormalizedModulatedValue(CHAOSD_PARAM, CHAOSD_INPUT, CHAOSD_SCALE_PARAM);

        fbSine.brokenMode = params[MODE_PARAM].getValue();

        fbSine.setIndexX(chaosAmountA);
		fbSine.setPhaseInc(chaosAmountB);
		fbSine.setPhaseX(chaosAmountC);
		fbSine.setFeedback(chaosAmountD);
		
		fbSine.generate();
		xVal = fbSine.outX;
		yVal = fbSine.outY;

        slewX.setTargetValue(xVal);
        slewY.setTargetValue(yVal);
    }

    if(params[SLEW_PARAM].getValue() == 1.0f)
    {
        slewX.setSRFactor(sRate.getSampleRateFactor());
        slewY.setSRFactor(sRate.getSampleRateFactor());
        xVal = slewX();
        yVal = slewY();
    }

    simd::float_4 filteredOut = {xVal, yVal, 0.0f, 0.0f};
    dcFilter.setFader(params[DC_PARAM].getValue());
    filteredOut = dcFilter.process(filteredOut);

    outputs[X_OUTPUT].setVoltage(filteredOut[0] * 5.0f);
    outputs[Y_OUTPUT].setVoltage(filteredOut[1] * 5.0f);
    
}


struct FBSineChaosWidget : HCVModuleWidget { FBSineChaosWidget(FBSineChaos *module); };

FBSineChaosWidget::FBSineChaosWidget(FBSineChaos *module)
{
	setSkinPath("res/FBSineChaos.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 35.0f;
    const float knobX = 70.0f;
    const float spacing = 45.0f;

    createParamComboHorizontal(knobX, knobY,                FBSineChaos::CHAOSA_PARAM, FBSineChaos::CHAOSA_SCALE_PARAM, FBSineChaos::CHAOSA_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing,      FBSineChaos::CHAOSB_PARAM, FBSineChaos::CHAOSB_SCALE_PARAM, FBSineChaos::CHAOSB_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0,  FBSineChaos::CHAOSC_PARAM, FBSineChaos::CHAOSC_SCALE_PARAM, FBSineChaos::CHAOSC_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*3.0,  FBSineChaos::CHAOSD_PARAM, FBSineChaos::CHAOSD_SCALE_PARAM, FBSineChaos::CHAOSD_INPUT);

    const float sRateX = 15.0f;
    const float sRateY = 60.0f;
    createParamComboVertical(sRateX, sRateY, FBSineChaos::SRATE_PARAM, FBSineChaos::SRATE_SCALE_PARAM, FBSineChaos::SRATE_INPUT);

    const float switchY = 238.0f;
    createHCVSwitchVert(19.0f, switchY, FBSineChaos::RANGE_PARAM);
    createHCVSwitchVert(80.0f, switchY, FBSineChaos::SLEW_PARAM);
    createHCVSwitchVert(143.0f, switchY, FBSineChaos::MODE_PARAM);
    createHCVSwitchVert(206.0f, switchY, FBSineChaos::DC_PARAM);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(50.0f, jackY, FBSineChaos::CLOCK_INPUT);

	//////OUTPUTS//////
    createOutputPort(134.0f, jackY, FBSineChaos::X_OUTPUT);
    createOutputPort(184.0f, jackY, FBSineChaos::Y_OUTPUT);
    
}

Model *modelFBSineChaos = createModel<FBSineChaos, FBSineChaosWidget>("FBSineChaos");
