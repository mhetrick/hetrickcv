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
        NUM_LIGHTS = 6
	};

	ClockedNoise()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ClockedNoise::SRATE_PARAM, 0.01, 1.0, 1.0, "Sample Rate");
		configParam(ClockedNoise::SRATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(ClockedNoise::FLUX_PARAM, -5.0, 5.0, -5.0, "Flux");
		configParam(ClockedNoise::FLUX_SCALE_PARAM, -1.0, 1.0, 1.0, "Flux CV Depth");

        configParam(ClockedNoise::MODE_PARAM, 0.0, 5.0, 0.0, "Mode");
		configParam(ClockedNoise::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");

        configSwitch(ClockedNoise::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(ClockedNoise::SLEW_PARAM, 0.0, 1.0, 0.0, "Enable Slew", {"Stepped", "Slewed"});
        configSwitch(ClockedNoise::DC_PARAM, 0.0, 1.0, 0.0, "DC Filtering", {"DC", "AC"});


        configInput(CLOCK_INPUT, "Clock");
        configInput(SRATE_INPUT, "Sample Rate CV");
        configInput(FLUX_INPUT, "Flux CV");

        configOutput(MAIN_OUTPUT, "Noise");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    float outVal = 0.0f;
    int mode = 0;

    float chaosAmount = 0.0f;

    bool bipolar = true;
    rack::dsp::SchmittTrigger clockTrigger;

    HCVSampleRate sRate;
    HCVSRateInterpolator slew;
    HCVDCFilterT<float> dcFilter;

    float fluxNoise = 0.0f;

    HCVRandom randGen;
    gam::NoiseWhite<> whiteNoise;
    gam::NoisePink<> pinkNoise;
    gam::NoiseBrown<> brownNoise;
    HCVLFSRNoise lfsrNoise;
    HCVGrayNoise grayNoise;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu

    void renderNoise()
    {
        switch(mode)
        {
            case 0: //white
                outVal = whiteNoise();
                break;
            
            case 1: //lfsr
                outVal = lfsrNoise();
                break;
                
            case 2: //gray
                outVal = grayNoise();
                break;
                
            case 3: //pink
                outVal = pinkNoise() * 2.0;
                break;
                
            case 4: //brown
                outVal = brownNoise();
                break;
                
            case 5: //gaussian
                outVal = randGen.nextGaussian();
                break;
                
            default:
            
                break;
        }

    }

};

void ClockedNoise::process(const ProcessArgs &args)
{
    auto fluxAmount = getNormalizedModulatedValue(FLUX_PARAM, FLUX_INPUT, FLUX_SCALE_PARAM);
    auto flux = fluxAmount * fluxNoise;

    float sr = params[SRATE_PARAM].getValue() + (inputs[SRATE_INPUT].getVoltage() * params[SRATE_SCALE_PARAM].getValue() * 0.2f);
    sr = clamp(sr + flux, 0.01f, 1.0f);
    float finalSr = sr*sr*sr;

    if(params[RANGE_PARAM].getValue() < 0.1f) finalSr = finalSr * 0.01f;
    sRate.setSampleRateFactor(finalSr);

    bool isReady = sRate.readyForNextSample();
    if(inputs[CLOCK_INPUT].isConnected()) isReady = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());

    float modeValue = params[MODE_PARAM].getValue() + (params[MODE_SCALE_PARAM].getValue() * inputs[MODE_INPUT].getVoltage());
    modeValue = clamp(modeValue, 0.0, 5.0);
    mode = (int) std::round(modeValue);

    if(isReady)
    {   
        fluxNoise = randGen.whiteNoise();

        renderNoise();
        slew.setTargetValue(outVal);
    }

    if(params[SLEW_PARAM].getValue() == 1.0f)
    {
        slew.setSRFactor(sRate.getSampleRateFactor());
        outVal = slew();
    }

    dcFilter.setFader(params[DC_PARAM].getValue());
    auto filteredOut = dcFilter.process(outVal);

    outputs[MAIN_OUTPUT].setVoltage(filteredOut * 5.0f);

    for (int i = 0; i < 6; i++)
    {
        lights[i].setBrightness(mode == i ? 1.0f : 0.0f);
    }
    
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
    for (int i = 0; i < 6; i++)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(130.0, 223 + (i*9.5)), module, i));
    }
    
}

Model *modelClockedNoise = createModel<ClockedNoise, ClockedNoiseWidget>("ClockedNoise");
