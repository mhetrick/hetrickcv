#include "HetrickCV.hpp"                                   
#include "DSP/HCVDCFilter.h"

struct TrigShaper : HCVModule
{
	enum ParamIds
	{
		FUNCTION_PARAM,
        MODE_PARAM,
        SCALE_PARAM,
        RANGE_PARAM,
        DCBLOCK_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        FUNCTION_INPUT,
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
        ENUMS(FUNCTION_LIGHTS, 3),
        ENUMS(MODE_LIGHTS, 3),
        NUM_LIGHTS
	};

	TrigShaper()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configSwitch(FUNCTION_PARAM, 0.0, 2.0, 0.0, "Function", 
        {"Sine", "Cosine", "Tangent"});
		configSwitch(MODE_PARAM, 0.0, 2.0, 0.0, "Mode", 
        {"Regular", "Hyperbolic", "Arc"});

        configParam(SCALE_PARAM, -1.0, 1.0, 1.0, "Input Scale");

		configSwitch(TrigShaper::RANGE_PARAM, 0.0, 1.0, 0.0, "Input Voltage Range", {"5V", "10V"});
        configSwitch(TrigShaper::DCBLOCK_PARAM, 0.0, 1.0, 0.0, "DC Blocking", {"DC", "AC"});

		configInput(FUNCTION_INPUT, "Function CV");
        configInput(MODE_INPUT, "Mode CV");

		configInput(MAIN_INPUT, "Main");
		configOutput(MAIN_OUTPUT, "Main");
	}

	void process(const ProcessArgs &args) override;

	HCVDCFilter dcFilters[16];

	float upscale = 5.0f;
	float downscale = 0.2f;
};


void TrigShaper::process(const ProcessArgs &args)
{
	const float functionKnob = params[FUNCTION_PARAM].getValue();
	const float modeKnob = params[MODE_PARAM].getValue();

	if (params[RANGE_PARAM].getValue() == 0.0f)
	{
		upscale = 5.0f;
		downscale = 0.2f;
	}
	else
	{
		upscale = 10.0f;
		downscale = 0.1f;
	}

    bool filterDC = params[DCBLOCK_PARAM].getValue() > 0.0f;

    const float inputScale = params[SCALE_PARAM].getValue() * downscale * PI;

	int numChannels = getMaxInputPolyphony();
	outputs[MAIN_OUTPUT].setChannels(numChannels);

	for (int chan = 0; chan < numChannels; chan++)
	{
        float input = inputs[MAIN_INPUT].getPolyVoltage(chan) * inputScale;
        input = clamp(input, -PI, PI);
        float hardClipped = clamp(input, -1.0f, 1.0f);

        float output = 0.0f;

        int function = functionKnob + inputs[FUNCTION_INPUT].getPolyVoltage(chan) * 0.4f;
        int mode = modeKnob + inputs[MODE_INPUT].getPolyVoltage(chan) * 0.4f;

        function = clamp(function, 0, 2);
        mode = clamp(mode, 0, 2);

        switch (function)
        {
        case 0: //SINE
            switch (mode)
            {
            case 0:
                output = sin(input);
                break;

            case 1:
                output = sinh(input);
                break;

            case 2:
                output = asin(hardClipped);
                break;
            
            default:
                break;
            }
            break;

        case 1: //COSINE
            switch (mode)
            {
            case 0:
                output = cos(input);
                break;

            case 1:
                output = cosh(input) - 2.0f;
                break;

            case 2:
                output = acos(hardClipped);
                break;
            
            default:
                break;
            }
            break;

        case 2: //TANGENT
            switch (mode)
            {
            case 0:
                output = tan(input);
                break;

            case 1:
                output = tanh(input);
                break;

            case 2:
                output = atan(input);
                break;
            
            default:
                break;
            }
            break;
        
        default:
            output = input;
            break;
        }

        if(!std::isnormal(output)) output = 0.0f;

        output = clamp(output, -1.0f, 1.0f);
        dcFilters[chan].setEnabled(filterDC);
        output = dcFilters[chan](output);
        outputs[MAIN_OUTPUT].setVoltage(output * upscale, chan);
	}

    int functionLight = functionKnob + inputs[FUNCTION_INPUT].getVoltage() * 0.4f;
    int modeLight = modeKnob + inputs[MODE_INPUT].getVoltage() * 0.4f;
    functionLight = clamp(functionLight, 0, 2);
    modeLight = clamp(modeLight, 0, 2);

    for (int i = FUNCTION_LIGHTS; i <= FUNCTION_LIGHTS_LAST; i++)
    {
        lights[i].setBrightness(functionLight == (i - FUNCTION_LIGHTS) ? 5.0f : 0.0f);
    }

    for (int i = MODE_LIGHTS; i <= MODE_LIGHTS_LAST; i++)
    {
        lights[i].setBrightness(modeLight == (i - MODE_LIGHTS) ? 5.0f : 0.0f);
    }
}


struct TrigShaperWidget : HCVModuleWidget { TrigShaperWidget(TrigShaper *module); };

TrigShaperWidget::TrigShaperWidget(TrigShaper *module)
{
	setSkinPath("res/TrigShaper.svg");
	initializeWidget(module);

	//////PARAMS//////
    int knobX = 45;
    addParam(createParam<RoundHugeBlackKnob>(Vec(knobX, 75), module, TrigShaper::FUNCTION_PARAM));
    addParam(createParam<RoundHugeBlackKnob>(Vec(knobX, 175), module, TrigShaper::MODE_PARAM));
	//createHCVKnob(knobX, 75, TrigShaper::FUNCTION_PARAM);
    //createHCVKnob(knobX, 175, TrigShaper::MODE_PARAM);

	createHCVTrimpot(55, 312, TrigShaper::SCALE_PARAM);

    int switchY = 260;
	createHCVSwitchVert(30, switchY, TrigShaper::RANGE_PARAM);
    createHCVSwitchVert(106, switchY, TrigShaper::DCBLOCK_PARAM);

	//////INPUTS//////
    int cvX = 10;
    addInput(createInput<PJ301MPort>(Vec(cvX, 90), module, TrigShaper::FUNCTION_INPUT));
    addInput(createInput<PJ301MPort>(Vec(cvX, 190), module, TrigShaper::MODE_INPUT));

    int mainJacksY = 310;
    addInput(createInput<PJ301MPort>(Vec(23, mainJacksY), module, TrigShaper::MAIN_INPUT));
    
	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(103, mainJacksY), module, TrigShaper::MAIN_OUTPUT));

    //////LIGHTS//////
    float lightX = 110.0f;
    float lightSpacing = 23.0f;
    for (int i = TrigShaper::FUNCTION_LIGHTS; i <= TrigShaper::FUNCTION_LIGHTS_LAST; i++)
    {
        float lightY = 73.0f + (i - TrigShaper::FUNCTION_LIGHTS)*lightSpacing;
        createHCVRedLight(lightX, lightY, i);
    }

    for (int i = TrigShaper::MODE_LIGHTS; i <= TrigShaper::MODE_LIGHTS_LAST; i++)
    {
        float lightY = 169.0f + (i - TrigShaper::MODE_LIGHTS)*lightSpacing;
        createHCVRedLight(lightX, lightY, i);
    }  
}

Model *modelTrigShaper = createModel<TrigShaper, TrigShaperWidget>("TrigShaper");
