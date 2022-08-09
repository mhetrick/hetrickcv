#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVCrackle.h"

struct Chaos2Op : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM, SRATE_SCALE_PARAM,
        CHAOSA_PARAM, CHAOSA_SCALE_PARAM,
        CHAOSB_PARAM, CHAOSB_SCALE_PARAM,
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
        NUM_LIGHTS = 5
	};

	Chaos2Op()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Chaos2Op::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(Chaos2Op::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(Chaos2Op::CHAOSA_PARAM, -5.0, 5.0, 5.0, "Chaos A");
		configParam(Chaos2Op::CHAOSA_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos A CV Depth");

        configParam(Chaos2Op::CHAOSB_PARAM, -5.0, 5.0, 0.0, "Chaos B");
		configParam(Chaos2Op::CHAOSB_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos B CV Depth");

        configParam(Chaos2Op::MODE_PARAM, 0.0, 4.0, 4.0, "Mode");
		configParam(Chaos2Op::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");

        configSwitch(Chaos2Op::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(Chaos2Op::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(Chaos2Op::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});

        configButton(Chaos2Op::RESEED_PARAM, "Reseed Button");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESEED_INPUT, "Reseed");
        configInput(SRATE_INPUT, "Sample Rate CV");

        configInput(CHAOSA_INPUT, "Chaos A CV");
        configInput(CHAOSB_INPUT, "Chaos B CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float xVal = 0.0f, yVal = 0.0f;
    int mode = 3;

    float chaosAmountA = 0.0f, chaosAmountB = 0.0f;

    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger, reseedTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slewX, slewY;
    HCVDCFilterT<simd::float_4> dcFilter;

    HCVCuspMap cusp;
    HCVGaussMap gauss;
    HCVHenonMap henon;
    HCVHetrickMap hetrick;
    HCVMouseMap mouse;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu

    void renderChaos()
    {
        switch(mode)
        {
            case 0: //cusp
                cusp.setChaosAmount(chaosAmountA, chaosAmountB);
                cusp.generate();
                xVal = cusp.out1;
                yVal = cusp.out2;
                break;
            
            case 1: //gauss
                gauss.setChaosAmount(chaosAmountA, chaosAmountB);
                gauss.generate();
                xVal = gauss.out1;
                yVal = gauss.out2;
                break;
                
            case 2: //henon
                henon.setChaosAmount(chaosAmountA, chaosAmountB);
                henon.generate();
                xVal = henon.out1;
                yVal = henon.out2;
                break;
                
            case 3: //hetrick
                hetrick.setChaosAmount(chaosAmountA, chaosAmountB);
                hetrick.generate();
                xVal = hetrick.out1;
                yVal = hetrick.out2;
                break;
                
            case 4: //mouse
                mouse.setChaosAmount(chaosAmountA, chaosAmountB);
                mouse.generate();
                xVal = mouse.out1;
                yVal = mouse.out2;
                break;
                
            default:
            
                break;
        }

    }

    void resetChaos()
    {
        switch(mode)
        {
            case 0: //cusp
                cusp.reset();
                break;
            
            case 1: //gauss
                gauss.reset();
                break;
                
            case 2: //henon
                henon.reset();
                break;
                
            case 3: //hetrick
                hetrick.reset();
                break;
                
            case 4: //mouse
                mouse.reset();
                break;
                
            default:
            
                break;
        }
        
        sRate.reset();
    }
};

void Chaos2Op::process(const ProcessArgs &args)
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
    modeValue = clamp(modeValue, 0.0, 4.0);
    mode = (int) std::round(modeValue);

    if(isReady)
    {   
        chaosAmountA = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM);
        chaosAmountB = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM);

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


struct Chaos2OpWidget : HCVModuleWidget { Chaos2OpWidget(Chaos2Op *module); };

Chaos2OpWidget::Chaos2OpWidget(Chaos2Op *module)
{
	setSkinPath("res/2OpChaos.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 35.0f;
    const float knobX = 10.0f;
    const float spacing = 45.0f;

    createParamComboHorizontal(knobX, knobY, Chaos2Op::SRATE_PARAM, Chaos2Op::SRATE_SCALE_PARAM, Chaos2Op::SRATE_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing, Chaos2Op::CHAOSA_PARAM, Chaos2Op::CHAOSA_SCALE_PARAM, Chaos2Op::CHAOSA_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0, Chaos2Op::CHAOSB_PARAM, Chaos2Op::CHAOSB_SCALE_PARAM, Chaos2Op::CHAOSB_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*3.0, Chaos2Op::MODE_PARAM, Chaos2Op::MODE_SCALE_PARAM, Chaos2Op::MODE_INPUT);


    const float switchY = 238.0f;
    createHCVSwitchVert(15.0f, switchY, Chaos2Op::RANGE_PARAM);
    createHCVSwitchVert(55.0f, switchY, Chaos2Op::SLEW_PARAM);
    createHCVSwitchVert(96.0f, switchY, Chaos2Op::DC_PARAM);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(11.0f, jackY, Chaos2Op::CLOCK_INPUT);
    createInputPort(56.0f, jackY, Chaos2Op::RESEED_INPUT);
    createHCVButtonSmall(60.0f, jackY - 20.0f, Chaos2Op::RESEED_PARAM);

	//////OUTPUTS//////
    createOutputPort(104.0f, jackY, Chaos2Op::X_OUTPUT);
    createOutputPort(146.0f, jackY, Chaos2Op::Y_OUTPUT);

    for (int i = 0; i < 5; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(130.0, 227 + (i*9.5)), module, i));
    }
    
}

Model *modelChaos2Op = createModel<Chaos2Op, Chaos2OpWidget>("Chaos2Op");
