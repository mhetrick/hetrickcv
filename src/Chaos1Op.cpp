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
        ENUMS(MODE_LIGHTS, 7),
        ENUMS(XOUT_LIGHT, 2),
        ENUMS(YOUT_LIGHT, 2),
        NUM_LIGHTS
	};

	Chaos1Op()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Chaos1Op::SRATE_PARAM, 0.01, 1.0, 0.5, "Sample Rate");
		configParam(Chaos1Op::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(Chaos1Op::CHAOS_PARAM, -5.0, 5.0, 0.0, "Chaos");
		configParam(Chaos1Op::CHAOS_SCALE_PARAM, -1.0, 1.0, 1.0, "Chaos CV Depth");
        
        configSwitch(Chaos1Op::MODE_PARAM, 0.0, 6.0, 0.0, "Mode",
        {"Crackle", "Broken Crackle", "Ikeda", "Logistic", "Standard", "Tent", "Thomas"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
		configParam(Chaos1Op::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");

        configSwitch(Chaos1Op::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(Chaos1Op::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(Chaos1Op::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});

        configButton(Chaos1Op::RESEED_PARAM, "Reseed Button");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESEED_INPUT, "Reseed");
        configInput(SRATE_INPUT, "Sample Rate CV");
        configInput(CHAOS_INPUT, "Chaos CV");
        configInput(MODE_INPUT, "Mode CV");

        configOutput(X_OUTPUT, "X");
        configOutput(Y_OUTPUT, "Y");

        random::init();
    }

    void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float xVal[16] = {}, yVal[16] = {};
    int mode[16] = {};
    float chaosAmount[16] = {};

    rack::dsp::SchmittTrigger clockTrigger[16], reseedTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slewX[16], slewY[16];
    HCVDCFilterT<simd::float_4> dcFilter[16];

    // Per-channel chaos generators
    HCVCrackle crackle[16];
    HCVLogisticMap logistic[16];
    HCVIkedaMap ikeda[16];
    HCVStandardMap standard[16];
    HCVTentMap tent[16];
    HCVThomasMap thomas[16];

    void renderChaos(int channel);
    void resetChaos(int channel);
};

void Chaos1Op::renderChaos(int channel)
{
    switch(mode[channel])
    {
        case 0: //crackle
            crackle[channel].setDensity(chaosAmount[channel]);
            crackle[channel].setBrokenMode(false);
            crackle[channel].generateStereo();
            xVal[channel] = crackle[channel].outL;
            yVal[channel] = crackle[channel].outR;
            break;
        
        case 1: //broken crackle
            crackle[channel].setDensity(chaosAmount[channel]);
            crackle[channel].setBrokenMode(true);
            crackle[channel].generateStereo();
            xVal[channel] = crackle[channel].outL;
            yVal[channel] = crackle[channel].outR;
            break;
            
        case 2: //ikeda
            ikeda[channel].setChaosAmount(chaosAmount[channel]);
            ikeda[channel].generate();
            xVal[channel] = ikeda[channel].out1;
            yVal[channel] = ikeda[channel].out2;
            break;
            
        case 3: //logistic
            logistic[channel].setChaosAmount(chaosAmount[channel]);
            logistic[channel].generate();
            xVal[channel] = logistic[channel].out1;
            yVal[channel] = logistic[channel].out2;
            break;
            
        case 4: //standard
            standard[channel].setChaosAmount(chaosAmount[channel]);
            standard[channel].generate();
            xVal[channel] = standard[channel].out1;
            yVal[channel] = standard[channel].out2;
            break;
            
        case 5: //tent
            tent[channel].setChaosAmount(chaosAmount[channel]);
            tent[channel].generate();
            xVal[channel] = tent[channel].out1;
            yVal[channel] = tent[channel].out2;
            break;

        case 6: //thomas
            thomas[channel].setChaosAmount(chaosAmount[channel]);
            thomas[channel].generate();
            xVal[channel] = thomas[channel].out1;
            yVal[channel] = thomas[channel].out2;
            break;

        default:            
            break;
    }
}

void Chaos1Op::resetChaos(int channel)
{
    switch(mode[channel])
    {
        case 0: //crackle
        case 1: //broken crackle
            crackle[channel].reset();
            break;
            
        case 2: //ikeda
            ikeda[channel].reset();
            break;
            
        case 3: //logistic
            logistic[channel].reset();
            break;
            
        case 4: //standard
            standard[channel].reset();
            break;
            
        case 5: //tent
            tent[channel].reset();
            break;

        case 6: //thomas
            thomas[channel].reset();
            break;
            
        default:
            break;
    }
    
    sRate[channel].reset();
}

void Chaos1Op::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getPolyVoltage(c) * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
        sr = clamp(sr, 0.01f, 1.0f);
        float finalSr = sr*sr*sr;

        if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
        sRate[c].setSampleRateFactor(finalSr);

        bool isReady = sRate[c].readyForNextSample();
        if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger[c].process(inputs[CLOCK_INPUT].getPolyVoltage(c));

        float modeValue = params[MODE_PARAM].getValue() + (params[MODE_SCALE_PARAM].getValue() * inputs[MODE_INPUT].getPolyVoltage(c));
        modeValue = clamp(modeValue, 0.0, 6.0);
        mode[c] = (int) std::round(modeValue);

        if(reseedTrigger[c].process(inputs[RESEED_INPUT].getPolyVoltage(c) + (c == 0 ? params[RESEED_PARAM].getValue() : 0.0f)))
        {
            resetChaos(c);
        }

        if(isReady)
        {   
            chaosAmount[c] = getNormalizedModulatedValue(CHAOS_PARAM, CHAOS_INPUT, CHAOS_SCALE_PARAM, c);

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


struct Chaos1OpWidget : HCVModuleWidget { Chaos1OpWidget(Chaos1Op *module); };

Chaos1OpWidget::Chaos1OpWidget(Chaos1Op *module)
{
	setSkinPath("res/Chaos1Op.svg");
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

    createHCVBipolarLightForJack(104.0f, jackY, Chaos1Op::XOUT_LIGHT);
    createHCVBipolarLightForJack(146.0f, jackY, Chaos1Op::YOUT_LIGHT);

    for (int i = 0; i <= Chaos1Op::MODE_LIGHTS_LAST; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(130.0, 223 + (i*9.5)), module, i));
    }
    
}

Model *modelChaos1Op = createModel<Chaos1Op, Chaos1OpWidget>("Chaos1Op");
