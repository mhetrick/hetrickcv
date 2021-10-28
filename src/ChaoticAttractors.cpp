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
        NUM_LIGHTS = 8
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

        configParam(ChaoticAttractors::MODE_PARAM, 0.0, 7.0, 4.0, "Mode");
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

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y");
        configOutput(Z_OUTPUT, "Z");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float xVal = 0.0f, yVal = 0.0f, zVal = 0.0f;
    int mode = 3;

    float chaosAmountA = 0.0f, chaosAmountB = 0.0f, chaosAmountC = 0.0f, chaosAmountD = 0.0f;

    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger, reseedTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slewX, slewY, slewZ;
    HCVDCFilterT<simd::float_4> dcFilter;

    HCVDeJongMap dejong;
    HCVLatoocarfianMap latoocarfian;
    HCVCliffordMap clifford;
    HCVTinkerbellMap tinkerbell;
    HCVLorenzMap lorenz;
    HCVRosslerMap rossler;
    HCVPickoverMap pickover;
    HCVFitzhughNagumoMap fitzhugh;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu

    void renderChaos()
    {
        switch(mode)
        {
            case 0: //dejong
                dejong.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                dejong.generate();
                xVal = dejong.outX;
                yVal = dejong.outY;
                zVal = dejong.outZ;
                break;
            
            case 1: //latoocarfian
                latoocarfian.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                latoocarfian.generate();
                xVal = latoocarfian.outX;
                yVal = latoocarfian.outY;
                zVal = latoocarfian.outZ;
                break;
                
            case 2: //clifford
                clifford.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                clifford.generate();
                xVal = clifford.outX;
                yVal = clifford.outY;
                zVal = clifford.outZ;
                break;
                
            case 3: //tinkerbell
                tinkerbell.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                tinkerbell.generate();
                xVal = tinkerbell.outX;
                yVal = tinkerbell.outY;
                zVal = tinkerbell.outZ;
                break;
                
            case 4: //lorenz
                lorenz.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                lorenz.generate();
                xVal = lorenz.outX;
                yVal = lorenz.outY;
                zVal = lorenz.outZ;
                break;
                
            case 5: //rossler
                rossler.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                rossler.generate();
                xVal = rossler.outX;
                yVal = rossler.outY;
                zVal = rossler.outZ;
                break;
            
            case 6: //pickover
                pickover.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                pickover.generate();
                xVal = pickover.outX;
                yVal = pickover.outY;
                zVal = pickover.outZ;
                break;
                
            case 7: //fitzhugh-nagumo
                fitzhugh.setChaosAmount(chaosAmountA, chaosAmountB, chaosAmountC, chaosAmountD);
                fitzhugh.generate();
                xVal = fitzhugh.outX;
                yVal = fitzhugh.outY;
                zVal = fitzhugh.outZ;
                break;
            
                
            default:
            
                break;
        }

    }

    void resetChaos()
    {
        switch(mode)
        {
            case 0: //dejong
                dejong.reset();
                break;
            
            case 1: //latoocarfian
                latoocarfian.reset();
                break;
                
            case 2: //clifford
                clifford.reset();
                break;
                
            case 3: //tinkerbell
                tinkerbell.reset();
                break;
                
            case 4: //lorenz
                lorenz.reset();
                break;
                
            case 5: //rossler
                rossler.reset();
                break;
            
            case 6: //pickover
                pickover.reset();
                break;
                
            case 7: //fitzhugh-nagumo
                fitzhugh.reset();
                break;
            
                
            default:
            
                break;
        }
        
        sRate.reset();
    }
};

void ChaoticAttractors::process(const ProcessArgs &args)
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

    float modeValue = params[MODE_PARAM].getValue() + (params[MODE_SCALE_PARAM].getValue() * inputs[MODE_INPUT].getVoltage() * 0.8f);
    modeValue = clamp(modeValue, 0.0, 7.0);
    mode = (int) std::round(modeValue);

    if(isReady)
    {   
        chaosAmountA = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM);
        chaosAmountB = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM);
        chaosAmountC = getNormalizedModulatedValue(CHAOSC_PARAM, CHAOSC_INPUT, CHAOSC_SCALE_PARAM);
        chaosAmountD = getNormalizedModulatedValue(CHAOSD_PARAM, CHAOSD_INPUT, CHAOSD_SCALE_PARAM);

        renderChaos();
        slewX.setTargetValue(xVal);
        slewY.setTargetValue(yVal);
        slewZ.setTargetValue(zVal);
    }

    if(params[SLEW_PARAM].getValue() == 1.0f)
    {
        slewX.setSRFactor(sRate.getSampleRateFactor());
        slewY.setSRFactor(sRate.getSampleRateFactor());
        slewZ.setSRFactor(sRate.getSampleRateFactor());
        xVal = slewX();
        yVal = slewY();
        zVal = slewZ();
    }

    simd::float_4 filteredOut = {xVal, yVal, zVal, 0.0f};
    dcFilter.setFader(params[DC_PARAM].getValue());
    filteredOut = dcFilter.process(filteredOut);

    outputs[X_OUTPUT].setVoltage(filteredOut[0] * 5.0f);
    outputs[Y_OUTPUT].setVoltage(filteredOut[1] * 5.0f);
    outputs[Z_OUTPUT].setVoltage(filteredOut[2] * 5.0f);

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[i].setBrightness(mode == i ? 1.0f : 0.0f);
    }
    
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

    for (int i = 0; i < ChaoticAttractors::NUM_LIGHTS; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(215.0, 212 + (i*9.5)), module, i));
    }
    
}

Model *modelChaoticAttractors = createModel<ChaoticAttractors, ChaoticAttractorsWidget>("ChaoticAttractors");
