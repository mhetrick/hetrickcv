#include "HetrickCV.hpp"
#include "DSP/HCVDCFilter.h"

struct AmplitudeShaper : HCVModule
{
	enum ParamIds
	{
		NEGATIVE_PARAM, NEGATIVE_SCALE_PARAM,
        POSITIVE_PARAM, POSITIVE_SCALE_PARAM,
        ALL_PARAM, ALL_SCALE_PARAM,
        
        DC_FILTER_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        NEGATIVE_INPUT,
        POSITIVE_INPUT,
        ALL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
        NEGATIVE_OUTPUT,
        POSITIVE_OUTPUT,
        NEGATIVE_GATE_OUTPUT,
        POSITIVE_GATE_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        NEGATIVE_GATE_LIGHT,
        POSITIVE_GATE_LIGHT,
        NUM_LIGHTS
	};

	AmplitudeShaper()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(NEGATIVE_PARAM, -1.0, 1.0, 1.0, "Negative Amplitude");
        configParam(NEGATIVE_SCALE_PARAM, -1.0, 1.0, 1.0, "Negative Amplitude CV Depth");

        configParam(POSITIVE_PARAM, -1.0, 1.0, 1.0, "Positive Amplitude");
        configParam(POSITIVE_SCALE_PARAM, -1.0, 1.0, 1.0, "Positive Amplitude CV Depth");

        configParam(ALL_PARAM, -1.0, 1.0, 1.0, "All Amplitude");
        configParam(ALL_SCALE_PARAM, -1.0, 1.0, 1.0, "All Amplitude CV Depth");

        configSwitch(DC_FILTER_PARAM, 0.0f, 1.0f, 0.0f, "DC Filter", {"Disabled", "Enabled"});
        
        configInput(MAIN_INPUT, "Main");
        configInput(NEGATIVE_INPUT, "Negative Amplitude CV");
        configInput(POSITIVE_INPUT, "Positive Amplitude CV");
        configInput(ALL_INPUT, "All Amplitude CV");

        configOutput(MAIN_OUTPUT, "Main");
        configOutput(NEGATIVE_OUTPUT, "Negative");
        configOutput(POSITIVE_OUTPUT, "Positive");
        configOutput(NEGATIVE_GATE_OUTPUT, "Negative Gate");
        configOutput(POSITIVE_GATE_OUTPUT, "Positive Gate");
	}

	void process(const ProcessArgs &args) override;
    
    HCVDCFilter dcFilter[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void AmplitudeShaper::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    
    const float negativeKnobValue = params[NEGATIVE_PARAM].getValue();
    const float positiveKnobValue = params[POSITIVE_PARAM].getValue();
    const float allKnobValue      = params[ALL_PARAM].getValue();

    const float negativeScaleKnobValue = params[NEGATIVE_SCALE_PARAM].getValue() * 0.2f;
    const float positiveScaleKnobValue = params[POSITIVE_SCALE_PARAM].getValue() * 0.2f;
    const float allScaleKnobValue      = params[ALL_SCALE_PARAM].getValue() * 0.2f;
    
    const bool dcFilterEnabled = params[DC_FILTER_PARAM].getValue() > 0.0f;
    
    for (int i = 0; i < numChannels; i++)
    {
        dcFilter[i].setEnabled(dcFilterEnabled);
        
        float positiveScale = inputs[POSITIVE_INPUT].getPolyVoltage(i) * positiveScaleKnobValue + positiveKnobValue;
        float negativeScale = inputs[NEGATIVE_INPUT].getPolyVoltage(i) * negativeScaleKnobValue + negativeKnobValue;
        float allScale      = inputs[ALL_INPUT].getPolyVoltage(i) * allScaleKnobValue + allKnobValue;
        
        positiveScale = clamp(positiveScale, -1.0f, 1.0f);
        negativeScale = clamp(negativeScale, -1.0f, 1.0f);
        allScale      = clamp(allScale, -1.0f, 1.0f);
        
        float input = inputs[MAIN_INPUT].getPolyVoltage(i) * allScale;
        bool isPositive = input >= 0.0f;
        
        if(isPositive)
        {
            const float outputValue = dcFilter[i](input * positiveScale);
            outputs[MAIN_OUTPUT].setVoltage(outputValue, i);
            outputs[POSITIVE_OUTPUT].setVoltage(outputValue, i);
            outputs[NEGATIVE_OUTPUT].setVoltage(0.0f, i);
            outputs[POSITIVE_GATE_OUTPUT].setVoltage(HCV_GATE_MAG, i);
            outputs[NEGATIVE_GATE_OUTPUT].setVoltage(0.0f, i);
        }
        else
        {
            const float outputValue = dcFilter[i](input * negativeScale);
            outputs[MAIN_OUTPUT].setVoltage(outputValue, i);
            outputs[NEGATIVE_OUTPUT].setVoltage(outputValue, i);
            outputs[POSITIVE_OUTPUT].setVoltage(0.0f, i);
            outputs[NEGATIVE_GATE_OUTPUT].setVoltage(HCV_GATE_MAG, i);
            outputs[POSITIVE_GATE_OUTPUT].setVoltage(0.0f, i);
        }
    }

    setLightFromOutput(NEGATIVE_GATE_LIGHT, NEGATIVE_GATE_OUTPUT);
    setLightFromOutput(POSITIVE_GATE_LIGHT, POSITIVE_GATE_OUTPUT);
}

struct AmplitudeShaperWidget : HCVModuleWidget { AmplitudeShaperWidget(AmplitudeShaper *module); };

AmplitudeShaperWidget::AmplitudeShaperWidget(AmplitudeShaper *module)
{
	setSkinPath("res/AmplitudeShaper.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, AmplitudeShaper::NEGATIVE_PARAM, AmplitudeShaper::NEGATIVE_SCALE_PARAM, AmplitudeShaper::NEGATIVE_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, AmplitudeShaper::POSITIVE_PARAM, AmplitudeShaper::POSITIVE_SCALE_PARAM, AmplitudeShaper::POSITIVE_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, AmplitudeShaper::ALL_PARAM, AmplitudeShaper::ALL_SCALE_PARAM, AmplitudeShaper::ALL_INPUT);

    const float topJackY = 270.0f;
    const float bottomJackY = 316.0f;
    const float leftX = 23.0f;
    const float middleX = 78.0f;
    const float rightX = 133.0f;
	//////INPUTS//////
    createInputPort(middleX, topJackY, AmplitudeShaper::MAIN_INPUT);

	//////OUTPUTS//////
    createOutputPort(middleX, bottomJackY, AmplitudeShaper::MAIN_OUTPUT);
    createOutputPort(leftX, bottomJackY, AmplitudeShaper::NEGATIVE_OUTPUT);
    createOutputPort(rightX, bottomJackY, AmplitudeShaper::POSITIVE_OUTPUT);
    
    //////GATES//////
    createOutputPort(leftX, topJackY, AmplitudeShaper::NEGATIVE_GATE_OUTPUT);
    createOutputPort(rightX, topJackY, AmplitudeShaper::POSITIVE_GATE_OUTPUT);

    createHCVRedLightForJack(leftX, topJackY, AmplitudeShaper::NEGATIVE_GATE_LIGHT);
    createHCVRedLightForJack(rightX, topJackY, AmplitudeShaper::POSITIVE_GATE_LIGHT);
    
    createHCVSwitchVert(middleX + 5, topJackY - 47, AmplitudeShaper::DC_FILTER_PARAM);
}

Model *modelAmplitudeShaper = createModel<AmplitudeShaper, AmplitudeShaperWidget>("AmplitudeShaper");
