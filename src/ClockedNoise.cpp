#include "HetrickCV.hpp"
#include "DSP/HCVRandom.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVShiftRegister.h"
#include "Gamma/Noise.h"

struct ClockedNoise : HCVModule
{
	enum ParamIds
	{
		SRATE_PARAM, SRATE_SCALE_PARAM,
        FLUX_PARAM, FLUX_SCALE_PARAM,
        MODE_PARAM, MODE_SCALE_PARAM,

        RANGE_PARAM,
        SLEW_PARAM,
        DC_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        SRATE_INPUT,
        FLUX_INPUT,
        MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {   
        ENUMS(MODE_LIGHTS, 6),
        ENUMS(OUT_LIGHT, 2),
        NUM_LIGHTS
	};

	ClockedNoise()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ClockedNoise::SRATE_PARAM, 0.01, 1.0, 1.0, "Sample Rate");
		configParam(ClockedNoise::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(ClockedNoise::FLUX_PARAM, -5.0, 5.0, -5.0, "Flux");
		configParam(ClockedNoise::FLUX_SCALE_PARAM, -1.0, 1.0, 1.0, "Flux CV Depth");

        configSwitch(ClockedNoise::MODE_PARAM, 0.0, 5.0, 0.0, "Mode", 
        {"White", "LFSR", "Gray", "Pink", "Brown", "Gaussian"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
		configParam(ClockedNoise::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");

        configSwitch(ClockedNoise::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(ClockedNoise::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(ClockedNoise::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});


        configInput(CLOCK_INPUT, "Clock");
        configInput(SRATE_INPUT, "Sample Rate CV");
        configInput(FLUX_INPUT, "Flux CV");
        configInput(MODE_INPUT, "Mode CV");

        configOutput(MAIN_OUTPUT, "Noise");

        random::init();
    }

	void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    float outVal[16] = {};
    int mode[16] = {};
    float fluxNoise[16] = {};

    rack::dsp::SchmittTrigger clockTrigger[16];

    HCVSampleRate sRate[16];
    HCVSRateInterpolator slew[16];
    HCVDCFilterT<float> dcFilter[16];

    // Per-channel noise generators
    HCVRandom randGen[16];
    gam::NoiseWhite<> whiteNoise[16];
    gam::NoisePink<> pinkNoise[16];
    gam::NoiseBrown<> brownNoise[16];
    HCVLFSRNoise lfsrNoise[16];
    HCVGrayNoise grayNoise[16];

    void renderNoise(int channel);

    // For more advanced Module features, read Rack's engine.hpp header file
    // - dataToJson, dataFromJson: serialization of internal data
    // - onSampleRateChange: event triggered by a change of sample rate
    // - reset, randomize: implements special behavior when user clicks these from the context menu
};

void ClockedNoise::renderNoise(int channel)
{
    switch(mode[channel])
    {
        case 0: //white
            outVal[channel] = whiteNoise[channel]();
            break;
        
        case 1: //lfsr
            outVal[channel] = lfsrNoise[channel]();
            break;
            
        case 2: //gray
            outVal[channel] = grayNoise[channel]();
            break;
            
        case 3: //pink
            outVal[channel] = pinkNoise[channel]() * 2.0;
            break;
            
        case 4: //brown
            outVal[channel] = brownNoise[channel]();
            break;
            
        case 5: //gaussian
            outVal[channel] = randGen[channel].nextGaussian();
            break;
            
        default:
            outVal[channel] = 0.0f;
            break;
    }
}

void ClockedNoise::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();
    
    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        auto fluxAmount = getNormalizedModulatedValue(FLUX_PARAM, FLUX_INPUT, FLUX_SCALE_PARAM, c);
        auto flux = fluxAmount * fluxNoise[c];

        float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getPolyVoltage(c) * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
        sr = clamp(sr + flux, 0.01f, 1.0f);
        float finalSr = sr*sr*sr;
        
        if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
        sRate[c].setSampleRateFactor(finalSr);
        
        bool isReady = sRate[c].readyForNextSample();
        if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger[c].process(inputs[CLOCK_INPUT].getPolyVoltage(c));

        float modeValue = params[MODE_PARAM].getValue() + (params[MODE_SCALE_PARAM].getValue() * inputs[MODE_INPUT].getPolyVoltage(c));
        modeValue = clamp(modeValue, 0.0, 5.0);
        mode[c] = (int) std::round(modeValue);

        if(isReady)
        {   
            fluxNoise[c] = randGen[c].whiteNoise();
            renderNoise(c);
            slew[c].setTargetValue(outVal[c]);
        }

        if(params[SLEW_PARAM].getValue() == 1.0f)
        {
            slew[c].setSRFactor(sRate[c].getSampleRateFactor());
            outVal[c] = slew[c]();
        }

        dcFilter[c].setFader(params[DC_PARAM].getValue());
        auto filteredOut = dcFilter[c].process(outVal[c]);

        outputs[MAIN_OUTPUT].setVoltage(filteredOut * 5.0f, c);
    }

    // Lights show the state of channel 0
    for (int i = 0; i <= MODE_LIGHTS_LAST; i++)
    {
        lights[MODE_LIGHTS + i].setBrightness(mode[0] == i ? 1.0f : 0.0f);
    }
    
    setBipolarLightBrightness(OUT_LIGHT, outputs[MAIN_OUTPUT].getVoltage(0) * 0.2f);
}


struct ClockedNoiseWidget : HCVModuleWidget { ClockedNoiseWidget(ClockedNoise *module); };

ClockedNoiseWidget::ClockedNoiseWidget(ClockedNoise *module)
{
	setSkinPath("res/ClockedNoise.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, ClockedNoise::SRATE_PARAM, ClockedNoise::SRATE_SCALE_PARAM, ClockedNoise::SRATE_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, ClockedNoise::FLUX_PARAM, ClockedNoise::FLUX_SCALE_PARAM, ClockedNoise::FLUX_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, ClockedNoise::MODE_PARAM, ClockedNoise::MODE_SCALE_PARAM, ClockedNoise::MODE_INPUT);


    const float switchY = 238.0f;
    createHCVSwitchVert(15.0f, switchY, ClockedNoise::RANGE_PARAM);
    createHCVSwitchVert(55.0f, switchY, ClockedNoise::SLEW_PARAM);
    createHCVSwitchVert(96.0f, switchY, ClockedNoise::DC_PARAM);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(46.0f, jackY, ClockedNoise::CLOCK_INPUT);

	//////OUTPUTS//////
    createOutputPort(116.0f, jackY, ClockedNoise::MAIN_OUTPUT);
    createHCVBipolarLightForJack(116.0f, jackY, ClockedNoise::OUT_LIGHT);

    for (int i = 0; i <= ClockedNoise::MODE_LIGHTS_LAST; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(130.0, 223 + (i*9.5)), module, i));
    }
    
}

Model *modelClockedNoise = createModel<ClockedNoise, ClockedNoiseWidget>("ClockedNoise");
