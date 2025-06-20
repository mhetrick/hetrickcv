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
        ENUMS(XOUT_LIGHT, 2),
        ENUMS(YOUT_LIGHT, 2),
        NUM_LIGHTS
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
        configInput(CHAOSD_INPUT, "Feedback CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y (Phase)");

        random::init();
    }

    void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float xVal[16] = {}, yVal[16] = {};
    float chaosAmountA[16] = {}, chaosAmountB[16] = {}, chaosAmountC[16] = {}, chaosAmountD[16] = {};

    rack::dsp::SchmittTrigger clockTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slewX[16], slewY[16];
    HCVDCFilterT<simd::float_4> dcFilter[16];

    // Per-channel chaos generators
    HCVFBSineChaos fbSine[16];

    // For more advanced Module features, read Rack's engine.hpp header file
    // - dataToJson, dataFromJson: serialization of internal data
    // - onSampleRateChange: event triggered by a change of sample rate
    // - reset, randomize: implements special behavior when user clicks these from the context menu
};

void FBSineChaos::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Global mode setting (front-panel switch)
    const bool brokenMode = (params[MODE_PARAM].getValue() > 0.0f);

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        const float sr = getSampleRateParameter(SRATE_PARAM, SRATE_INPUT, SRATE_SCALE_PARAM, RANGE_PARAM, c);
        sRate[c].setSampleRateFactor(sr);

        bool isReady = sRate[c].readyForNextSample();
        if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger[c].process(inputs[CLOCK_INPUT].getPolyVoltage(c));

        if(isReady)
        {   
            chaosAmountA[c] = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM, c);
            chaosAmountB[c] = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM, c);
            chaosAmountC[c] = getNormalizedModulatedValue(CHAOSC_PARAM, CHAOSC_INPUT, CHAOSC_SCALE_PARAM, c);
            chaosAmountD[c] = getNormalizedModulatedValue(CHAOSD_PARAM, CHAOSD_INPUT, CHAOSD_SCALE_PARAM, c);

            fbSine[c].brokenMode = brokenMode;

            fbSine[c].setIndexX(chaosAmountA[c]);
            fbSine[c].setPhaseInc(chaosAmountB[c]);
            fbSine[c].setPhaseX(chaosAmountC[c]);
            fbSine[c].setFeedback(chaosAmountD[c]);
            
            fbSine[c].generate();
            xVal[c] = fbSine[c].outX;
            yVal[c] = fbSine[c].outY;

            slewX[c].setTargetValue(xVal[c]);
            slewY[c].setTargetValue(yVal[c]);
        }

        if(params[SLEW_PARAM].getValue() == 1.0f)
        {
            slewX[c].setSRFactor(sRate[c].getSampleRateFactor());
            slewY[c].setSRFactor(sRate[c].getSampleRateFactor());
            xVal[c] = slewX[c]();
            yVal[c] = slewY[c]();
        }

        simd::float_4 filteredOut = {xVal[c], yVal[c], 0.0f, 0.0f};
        dcFilter[c].setFader(params[DC_PARAM].getValue());
        filteredOut = dcFilter[c].process(filteredOut);

        outputs[X_OUTPUT].setVoltage(filteredOut[0] * 5.0f, c);
        outputs[Y_OUTPUT].setVoltage(filteredOut[1] * 5.0f, c);
    }
    
    // Lights show the state of channel 0
    setBipolarLightBrightness(XOUT_LIGHT, outputs[X_OUTPUT].getVoltage(0) * 0.2f);
    setBipolarLightBrightness(YOUT_LIGHT, outputs[Y_OUTPUT].getVoltage(0) * 0.2f);
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
    
    createHCVBipolarLightForJack(134.0f, jackY, FBSineChaos::XOUT_LIGHT);
    createHCVBipolarLightForJack(184.0f, jackY, FBSineChaos::YOUT_LIGHT);
}

Model *modelFBSineChaos = createModel<FBSineChaos, FBSineChaosWidget>("FBSineChaos");
