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
        ENUMS(MODE_LIGHTS, 5),
        ENUMS(XOUT_LIGHT, 2),
        ENUMS(YOUT_LIGHT, 2),
        NUM_LIGHTS
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

        configSwitch(Chaos2Op::MODE_PARAM, 0.0, 4.0, 4.0, "Mode", {"Cusp", "Gauss", "Henon", "Hetrick", "Mouse"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
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
        configInput(MODE_INPUT, "Mode CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float xVal[16] = {}, yVal[16] = {};
    int mode[16] = {};
    float chaosAmountA[16] = {}, chaosAmountB[16] = {};

    rack::dsp::SchmittTrigger clockTrigger[16], reseedTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slewX[16], slewY[16];
    HCVDCFilterT<simd::float_4> dcFilter[16];

    // Per-channel chaos generators
    HCVCuspMap cusp[16];
    HCVGaussMap gauss[16];
    HCVHenonMap henon[16];
    HCVHetrickMap hetrick[16];
    HCVMouseMap mouse[16];

    void renderChaos(int channel);
    void resetChaos(int channel);
};

void Chaos2Op::renderChaos(int channel)
{
    switch(mode[channel])
    {
        case 0: //cusp
            cusp[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel]);
            cusp[channel].generate();
            xVal[channel] = cusp[channel].out1;
            yVal[channel] = cusp[channel].out2;
            break;
        
        case 1: //gauss
            gauss[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel]);
            gauss[channel].generate();
            xVal[channel] = gauss[channel].out1;
            yVal[channel] = gauss[channel].out2;
            break;
            
        case 2: //henon
            henon[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel]);
            henon[channel].generate();
            xVal[channel] = henon[channel].out1;
            yVal[channel] = henon[channel].out2;
            break;
            
        case 3: //hetrick
            hetrick[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel]);
            hetrick[channel].generate();
            xVal[channel] = hetrick[channel].out1;
            yVal[channel] = hetrick[channel].out2;
            break;
            
        case 4: //mouse
            mouse[channel].setChaosAmount(chaosAmountA[channel], chaosAmountB[channel]);
            mouse[channel].generate();
            xVal[channel] = mouse[channel].out1;
            yVal[channel] = mouse[channel].out2;
            break;
            
        default:
            break;
    }
}

void Chaos2Op::resetChaos(int channel)
{
    switch(mode[channel])
    {
        case 0: //cusp
            cusp[channel].reset();
            break;
        
        case 1: //gauss
            gauss[channel].reset();
            break;
            
        case 2: //henon
            henon[channel].reset();
            break;
            
        case 3: //hetrick
            hetrick[channel].reset();
            break;
            
        case 4: //mouse
            mouse[channel].reset();
            break;
            
        default:
            break;
    }
    
    sRate[channel].reset();
}

void Chaos2Op::process(const ProcessArgs &args)
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
        modeValue = clamp(modeValue, 0.0, 4.0);
        mode[c] = (int) std::round(modeValue);

        if(isReady)
        {   
            chaosAmountA[c] = getNormalizedModulatedValue(CHAOSA_PARAM, CHAOSA_INPUT, CHAOSA_SCALE_PARAM, c);
            chaosAmountB[c] = getNormalizedModulatedValue(CHAOSB_PARAM, CHAOSB_INPUT, CHAOSB_SCALE_PARAM, c);

            renderChaos(c);
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
    for (int i = 0; i <= MODE_LIGHTS_LAST; i++)
    {
        lights[MODE_LIGHTS + i].setBrightness(mode[0] == i ? 1.0f : 0.0f);
    }

    setBipolarLightBrightness(XOUT_LIGHT, outputs[X_OUTPUT].getVoltage(0) * 0.2f);
    setBipolarLightBrightness(YOUT_LIGHT, outputs[Y_OUTPUT].getVoltage(0) * 0.2f);
}


struct Chaos2OpWidget : HCVModuleWidget { Chaos2OpWidget(Chaos2Op *module); };

Chaos2OpWidget::Chaos2OpWidget(Chaos2Op *module)
{
	setSkinPath("res/Chaos2Op.svg");
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

    createHCVBipolarLightForJack(104.0f, jackY, Chaos2Op::XOUT_LIGHT);
    createHCVBipolarLightForJack(146.0f, jackY, Chaos2Op::YOUT_LIGHT);

    for (int i = 0; i <= Chaos2Op::MODE_LIGHTS_LAST; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(130.0, 227 + (i*9.5)), module, i));
    }
    
}

Model *modelChaos2Op = createModel<Chaos2Op, Chaos2OpWidget>("Chaos2Op");
