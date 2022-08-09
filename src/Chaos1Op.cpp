#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVCrackle.h"

struct Chaos1Op : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM, SRATE_SCALE_PARAM,
        CHAOS_PARAM, CHAOS_SCALE_PARAM,
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
        CHAOS_INPUT,
        MODE_INPUT,
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
        NUM_LIGHTS = 6
	};

	Chaos1Op()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Chaos1Op::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(Chaos1Op::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(Chaos1Op::CHAOS_PARAM, -5.0, 5.0, 0.0, "Chaos");
		configParam(Chaos1Op::CHAOS_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos CV Depth");

        configParam(Chaos1Op::MODE_PARAM, 0.0, 5.0, 0.0, "Mode");
		configParam(Chaos1Op::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");

        configSwitch(Chaos1Op::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(Chaos1Op::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(Chaos1Op::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});

        configButton(Chaos1Op::RESEED_PARAM, "Reseed Button");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESEED_INPUT, "Reseed");
        configInput(SRATE_INPUT, "Sample Rate CV");

        configInput(CHAOS_INPUT, "Chaos CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float xVal = 0.0f, yVal = 0.0f;
    int mode = 3;

    float chaosAmount = 0.0f;

    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger, reseedTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slewX, slewY;
    HCVDCFilterT<simd::float_4> dcFilter;

    HCVCrackle crackle;
    HCVLogisticMap logistic;
    HCVIkedaMap ikeda;
    HCVStandardMap standard;
    HCVTentMap tent;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu

    void renderChaos()
    {
        switch(mode)
        {
            case 0: //crackle
                crackle.setDensity(chaosAmount);
                crackle.setBrokenMode(false);
                crackle.generateStereo();
                xVal = crackle.outL;
                yVal = crackle.outR;
                break;
            
            case 1: //broken crackle
                crackle.setDensity(chaosAmount);
                crackle.setBrokenMode(true);
                crackle.generateStereo();
                xVal = crackle.outL;
                yVal = crackle.outR;
                break;
                
            case 2: //ikeda
                ikeda.setChaosAmount(chaosAmount);
                ikeda.generate();
                xVal = ikeda.out1;
                yVal = ikeda.out2;
                break;
                
            case 3: //logistic
                logistic.setChaosAmount(chaosAmount);
                logistic.generate();
                xVal = logistic.out1;
                yVal = logistic.out2;
                break;
                
            case 4: //standard
                standard.setChaosAmount(chaosAmount);
                standard.generate();
                xVal = standard.out1;
                yVal = standard.out2;
                break;
                
            case 5: //tent
                tent.setChaosAmount(chaosAmount);
                tent.generate();
                xVal = tent.out1;
                yVal = tent.out2;
                break;
                
            default:
            
                break;
        }

    }

    void resetChaos()
    {
        switch(mode)
        {
            case 0: //crackle
                crackle.reset();
                break;
            
            case 1: //broken crackle
                crackle.reset();
                break;
                
            case 2: //ikeda
                ikeda.reset();
                break;
                
            case 3: //logistic
                logistic.reset();
                break;
                
            case 4: //standard
                standard.reset();
                break;
                
            case 5: //tent
                tent.reset();
                break;
                
            default:
            
                break;
        }
        
        sRate.reset();
    }
};

void Chaos1Op::process(const ProcessArgs &args)
{
    float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getVoltage() * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
    sr = clamp(sr, 0.01f, 1.0f);
    float finalSr = sr*sr*sr;

    if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
    sRate.setSampleRateFactor(finalSr);

    bool isReady = sRate.readyForNextSample();
    if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());

    float modeValue = params[MODE_PARAM].getValue() + (params[MODE_SCALE_PARAM].getValue() * inputs[MODE_INPUT].getVoltage());
    modeValue = clamp(modeValue, 0.0, 5.0);
    mode = (int) std::round(modeValue);

    if(reseedTrigger.process(inputs[RESEED_INPUT].getVoltage() + params[RESEED_PARAM].getValue()))
    {
        resetChaos();
        sRate.reset();
    }

    if(isReady)
    {   
        chaosAmount = getNormalizedModulatedValue(CHAOS_PARAM, CHAOS_INPUT, CHAOS_SCALE_PARAM);

        renderChaos();
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

    for (int i = 0; i < 6; i++)
    {
        lights[i].setBrightness(mode == i ? 1.0f : 0.0f);
    }
    
}


struct Chaos1OpWidget : HCVModuleWidget { Chaos1OpWidget(Chaos1Op *module); };

Chaos1OpWidget::Chaos1OpWidget(Chaos1Op *module)
{
	setSkinPath("res/1OpChaos.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, Chaos1Op::SRATE_PARAM, Chaos1Op::SRATE_SCALE_PARAM, Chaos1Op::SRATE_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, Chaos1Op::CHAOS_PARAM, Chaos1Op::CHAOS_SCALE_PARAM, Chaos1Op::CHAOS_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, Chaos1Op::MODE_PARAM, Chaos1Op::MODE_SCALE_PARAM, Chaos1Op::MODE_INPUT);


    const float switchY = 238.0f;
    createHCVSwitchVert(15.0f, switchY, Chaos1Op::RANGE_PARAM);
    createHCVSwitchVert(55.0f, switchY, Chaos1Op::SLEW_PARAM);
    createHCVSwitchVert(96.0f, switchY, Chaos1Op::DC_PARAM);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(11.0f, jackY, Chaos1Op::CLOCK_INPUT);
    createInputPort(56.0f, jackY, Chaos1Op::RESEED_INPUT);
    createHCVButtonSmall(60.0f, jackY - 20.0f, Chaos1Op::RESEED_PARAM);

	//////OUTPUTS//////
    createOutputPort(104.0f, jackY, Chaos1Op::X_OUTPUT);
    createOutputPort(146.0f, jackY, Chaos1Op::Y_OUTPUT);

    for (int i = 0; i < 6; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(130.0, 223 + (i*9.5)), module, i));
    }
    
}

Model *modelChaos1Op = createModel<Chaos1Op, Chaos1OpWidget>("Chaos1Op");
