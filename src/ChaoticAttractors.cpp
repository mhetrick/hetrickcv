#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVCrackle.h"

struct ChaoticAttractors : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM, SRATE_SCALE_PARAM,

        CHAOSA_PARAM, CHAOSA_SCALE_PARAM,
        CHAOSB_PARAM, CHAOSB_SCALE_PARAM,
        CHAOSC_PARAM, CHAOSC_SCALE_PARAM,
        CHAOSD_PARAM, CHAOSD_SCALE_PARAM,

        MODE_PARAM, MODE_SCALE_PARAM,

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

        CHAOSA_INPUT,
        CHAOSB_INPUT,
        CHAOSC_INPUT,
        CHAOSD_INPUT,
        
        MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		X_OUTPUT,
        Y_OUTPUT,
        Z_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {   
        ENUMS(MODE_LIGHTS, 8),
        ENUMS(XOUT_LIGHT, 2),
        ENUMS(YOUT_LIGHT, 2),
        ENUMS(ZOUT_LIGHT, 2),
        NUM_LIGHTS
	};

	ChaoticAttractors()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ChaoticAttractors::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(ChaoticAttractors::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(ChaoticAttractors::CHAOSA_PARAM, -5.0, 5.0, 5.0, "Chaos A");
		configParam(ChaoticAttractors::CHAOSA_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos A CV Depth");

        configParam(ChaoticAttractors::CHAOSB_PARAM, -5.0, 5.0, 0.0, "Chaos B");
		configParam(ChaoticAttractors::CHAOSB_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos B CV Depth");

        configParam(ChaoticAttractors::CHAOSC_PARAM, -5.0, 5.0, 5.0, "Chaos C");
		configParam(ChaoticAttractors::CHAOSC_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos C CV Depth");

        configParam(ChaoticAttractors::CHAOSD_PARAM, -5.0, 5.0, 0.0, "Chaos D");
		configParam(ChaoticAttractors::CHAOSD_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos D CV Depth");

        configSwitch(ChaoticAttractors::MODE_PARAM, 0.0, 7.0, 4.0, "Mode",
        {"De Jong", "Latoocarfian", "Clifford", "Tinkerbell", "Lorenz", "Rossler", "Pickover", "Fitzhugh-Nagumo"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
		configParam(ChaoticAttractors::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");

        configSwitch(ChaoticAttractors::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(ChaoticAttractors::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(ChaoticAttractors::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});

        configButton(ChaoticAttractors::RESEED_PARAM, "Reseed Button");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESEED_INPUT, "Reseed");
        configInput(SRATE_INPUT, "Sample Rate CV");

        configInput(CHAOSA_INPUT, "Chaos A CV");
        configInput(CHAOSB_INPUT, "Chaos B CV");
        configInput(CHAOSC_INPUT, "Chaos C CV");
        configInput(CHAOSD_INPUT, "Chaos D CV");

        configInput(MODE_INPUT, "Mode CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y");
        configOutput(Z_OUTPUT, "Z");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float xVal[16] = {}, yVal[16] = {}, zVal[16] = {};
    int mode[16] = {};
    float chaosAmountA[16] = {}, chaosAmountB[16] = {}, chaosAmountC[16] = {}, chaosAmountD[16] = {};

    rack::dsp::SchmittTrigger clockTrigger[16], reseedTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slewX[16], slewY[16], slewZ[16];
    HCVDCFilterT<simd::float_4> dcFilter[16];

    // Per-channel chaos generators
    HCVDeJongMap dejong[16];
    HCVLatoocarfianMap latoocarfian[16];
    HCVCliffordMap clifford[16];
    HCVTinkerbellMap tinkerbell[16];
    HCVLorenzMap lorenz[16];
    HCVRosslerMap rossler[16];
    HCVPickoverMap pickover[16];
    HCVFitzhughNagumoMap fitzhugh[16];

    void renderChaos(int channel);
    void resetChaos(int channel);
};

void ChaoticAttractors::renderChaos(int channel)
{
    switch(mode[channel])
    {
        case 0: //dejong
            dejong[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            dejong[channel].generate();
            xVal[channel] = dejong[channel].outX;
            yVal[channel] = dejong[channel].outY;
            zVal[channel] = dejong[channel].outZ;
            break;
        
        case 1: //latoocarfian
            latoocarfian[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            latoocarfian[channel].generate();
            xVal[channel] = latoocarfian[channel].outX;
            yVal[channel] = latoocarfian[channel].outY;
            zVal[channel] = latoocarfian[channel].outZ;
            break;
            
        case 2: //clifford
            clifford[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            clifford[channel].generate();
            xVal[channel] = clifford[channel].outX;
            yVal[channel] = clifford[channel].outY;
            zVal[channel] = clifford[channel].outZ;
            break;
            
        case 3: //tinkerbell
            tinkerbell[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            tinkerbell[channel].generate();
            xVal[channel] = tinkerbell[channel].outX;
            yVal[channel] = tinkerbell[channel].outY;
            zVal[channel] = tinkerbell[channel].outZ;
            break;
            
        case 4: //lorenz
            lorenz[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            lorenz[channel].generate();
            xVal[channel] = lorenz[channel].outX;
            yVal[channel] = lorenz[channel].outY;
            zVal[channel] = lorenz[channel].outZ;
            break;
            
        case 5: //rossler
            rossler[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            rossler[channel].generate();
            xVal[channel] = rossler[channel].outX;
            yVal[channel] = rossler[channel].outY;
            zVal[channel] = rossler[channel].outZ;
            break;
        
        case 6: //pickover
            pickover[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            pickover[channel].generate();
            xVal[channel] = pickover[channel].outX;
            yVal[channel] = pickover[channel].outY;
            zVal[channel] = pickover[channel].outZ;
            break;
            
        case 7: //fitzhugh-nagumo
            fitzhugh[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel], chaosAmountC[channel], chaosAmountD[channel]);
            fitzhugh[channel].generate();
            xVal[channel] = fitzhugh[channel].outX;
            yVal[channel] = fitzhugh[channel].outY;
            zVal[channel] = fitzhugh[channel].outZ;
            break;
            
        default:
            break;
    }
}

void ChaoticAttractors::resetChaos(int channel)
{
    switch(mode[channel])
    {
        case 0: //dejong
            dejong[channel].reset();
            break;
        
        case 1: //latoocarfian
            latoocarfian[channel].reset();
            break;
            
        case 2: //clifford
            clifford[channel].reset();
            break;
            
        case 3: //tinkerbell
            tinkerbell[channel].reset();
            break;
            
        case 4: //lorenz
            lorenz[channel].reset();
            break;
            
        case 5: //rossler
            rossler[channel].reset();
            break;
        
        case 6: //pickover
            pickover[channel].reset();
            break;
            
        case 7: //fitzhugh-nagumo
            fitzhugh[channel].reset();
            break;
            
        default:
            break;
    }
    
    sRate[channel].reset();
}

void ChaoticAttractors::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

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

        float modeValue = params[MODE_PARAM].getValue() + (params[MODE_SCALE_PARAM].getValue() * inputs[MODE_INPUT].getPolyVoltage(c) * 0.8f);
        modeValue = clamp(modeValue, 0.0, 7.0);
        mode[c] = (int) std::round(modeValue);

        if(isReady)
        {   
            chaosAmountA[c] = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM, c);
            chaosAmountB[c] = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM, c);
            chaosAmountC[c] = getNormalizedModulatedValue(CHAOSC_PARAM, CHAOSC_INPUT, CHAOSC_SCALE_PARAM, c);
            chaosAmountD[c] = getNormalizedModulatedValue(CHAOSD_PARAM, CHAOSD_INPUT, CHAOSD_SCALE_PARAM, c);

            renderChaos(c);
            slewX[c].setTargetValue(xVal[c]);
            slewY[c].setTargetValue(yVal[c]);
            slewZ[c].setTargetValue(zVal[c]);
        }

        if(params[SLEW_PARAM].getValue() == 1.0f)
        {
            slewX[c].setSRFactor(sRate[c].getSampleRateFactor());
            slewY[c].setSRFactor(sRate[c].getSampleRateFactor());
            slewZ[c].setSRFactor(sRate[c].getSampleRateFactor());
            xVal[c] = slewX[c]();
            yVal[c] = slewY[c]();
            zVal[c] = slewZ[c]();
        }

        simd::float_4 filteredOut = {xVal[c], yVal[c], zVal[c], 0.0f};
        dcFilter[c].setFader(params[DC_PARAM].getValue());
        filteredOut = dcFilter[c].process(filteredOut);

        outputs[X_OUTPUT].setVoltage(filteredOut[0] * 5.0f, c);
        outputs[Y_OUTPUT].setVoltage(filteredOut[1] * 5.0f, c);
        outputs[Z_OUTPUT].setVoltage(filteredOut[2] * 5.0f, c);
    }

    // Lights show the state of channel 0
    for (int i = 0; i <= MODE_LIGHTS_LAST; i++)
    {
        lights[MODE_LIGHTS + i].setBrightness(mode[0] == i ? 1.0f : 0.0f);
    }
    
    setBipolarLightBrightness(XOUT_LIGHT, outputs[X_OUTPUT].getVoltage(0) * 0.2f);
    setBipolarLightBrightness(YOUT_LIGHT, outputs[Y_OUTPUT].getVoltage(0) * 0.2f);
    setBipolarLightBrightness(ZOUT_LIGHT, outputs[Z_OUTPUT].getVoltage(0) * 0.2f);
}


struct ChaoticAttractorsWidget : HCVModuleWidget { ChaoticAttractorsWidget(ChaoticAttractors *module); };

ChaoticAttractorsWidget::ChaoticAttractorsWidget(ChaoticAttractors *module)
{
	setSkinPath("res/ChaoticAttractors.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 35.0f;
    const float knobX = 70.0f;
    const float spacing = 45.0f;

    createParamComboHorizontal(knobX, knobY,                ChaoticAttractors::CHAOSA_PARAM, ChaoticAttractors::CHAOSA_SCALE_PARAM, ChaoticAttractors::CHAOSA_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing,      ChaoticAttractors::CHAOSB_PARAM, ChaoticAttractors::CHAOSB_SCALE_PARAM, ChaoticAttractors::CHAOSB_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0,  ChaoticAttractors::CHAOSC_PARAM, ChaoticAttractors::CHAOSC_SCALE_PARAM, ChaoticAttractors::CHAOSC_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*3.0,  ChaoticAttractors::CHAOSD_PARAM, ChaoticAttractors::CHAOSD_SCALE_PARAM, ChaoticAttractors::CHAOSD_INPUT);

    const float sRateX = 21.0f;
    const float sRateY = 60.0f;
    createParamComboVertical(sRateX, sRateY, ChaoticAttractors::SRATE_PARAM, ChaoticAttractors::SRATE_SCALE_PARAM, ChaoticAttractors::SRATE_INPUT);
    createParamComboVertical(sRateX + 224.0f, sRateY, ChaoticAttractors::MODE_PARAM, ChaoticAttractors::MODE_SCALE_PARAM, ChaoticAttractors::MODE_INPUT);

    const float switchY = 238.0f;
    createHCVSwitchVert(29.0f, switchY, ChaoticAttractors::RANGE_PARAM);
    createHCVSwitchVert(85.0f, switchY, ChaoticAttractors::SLEW_PARAM);
    createHCVSwitchVert(142.0f, switchY, ChaoticAttractors::DC_PARAM);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(23.0f, jackY, ChaoticAttractors::CLOCK_INPUT);
    createInputPort(79.0f, jackY, ChaoticAttractors::RESEED_INPUT);
    createHCVButtonSmall(83.0f, jackY - 20.0f, ChaoticAttractors::RESEED_PARAM);

	//////OUTPUTS//////
    createOutputPort(151.0f, jackY, ChaoticAttractors::X_OUTPUT);
    createOutputPort(203.0f, jackY, ChaoticAttractors::Y_OUTPUT);
    createOutputPort(254.0f, jackY, ChaoticAttractors::Z_OUTPUT);

    createHCVBipolarLightForJack(151.0f, jackY, ChaoticAttractors::XOUT_LIGHT);
    createHCVBipolarLightForJack(203.0f, jackY, ChaoticAttractors::YOUT_LIGHT);
    createHCVBipolarLightForJack(254.0f, jackY, ChaoticAttractors::ZOUT_LIGHT);

    for (int i = 0; i <= ChaoticAttractors::MODE_LIGHTS_LAST; i++)
    {
        createHCVRedLight(215.0f, 212 + (i*9.5f), ChaoticAttractors::MODE_LIGHTS + i);
    }
    
}

Model *modelChaoticAttractors = createModel<ChaoticAttractors, ChaoticAttractorsWidget>("ChaoticAttractors");
