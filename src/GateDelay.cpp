#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h"

struct GateDelay : HCVModule
{
	enum ParamIds
	{
        DELAY_PARAM,
        DELAYCV_PARAM,
        WIDTH_PARAM,
        WIDTHCV_PARAM,
        GATEBUTTON_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        GATE1_INPUT,
        GATE2_INPUT,
        DELAYCV_INPUT,
        WIDTHCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        DELAY_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        DELAY_LIGHT,

        NUM_LIGHTS
	};

    dsp::SchmittTrigger clockTrigger[16];
    HCVTriggerDelay delayGates[16];
    float gateOuts[16];
    const float maxTime = 5.0f;

	GateDelay()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(GateDelay::DELAY_PARAM, 0.0, maxTime, 0.1, "Delay Time", " s");
        configParam(GateDelay::DELAYCV_PARAM, -1.0, 1.0, 1.0, "Delay Time CV Depth");

        configParam(GateDelay::WIDTH_PARAM, 0.0001, maxTime, 0.1, "Gate Width", " s");
        configParam(GateDelay::WIDTHCV_PARAM, -1.0, 1.0, 1.0, "Gate Width CV Depth");

        configButton(GATEBUTTON_PARAM, "Gate Button");

        configInput(GateDelay::GATE1_INPUT, "Gate 1");
        configInput(GateDelay::GATE2_INPUT, "Gate 2");
        configInput(GateDelay::DELAYCV_INPUT, "Delay Time CV");
        configInput(GateDelay::WIDTHCV_INPUT, "Gate Width CV");

        configOutput(GateDelay::DELAY_OUTPUT, "Delayed Gate");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {
        for(int i = 0; i < 16; i++)
        {
            clockTrigger[i].reset();
            delayGates[i].reset();
            gateOuts[i] = 0.0f;
        }
    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void GateDelay::process(const ProcessArgs &args)
{
    const float delayKnob = params[DELAY_PARAM].getValue();
    const float delayDepth = params[DELAYCV_PARAM].getValue();

    const float widthKnob = params[WIDTH_PARAM].getValue();
    const float widthDepth = params[WIDTHCV_PARAM].getValue();

    const float gateButton = params[GATEBUTTON_PARAM].getValue() * 2.0f;

    outputs[DELAY_OUTPUT].setChannels(getMaxInputPolyphony());

    for (int i = 0; i < getMaxInputPolyphony(); i++)
    {
        float allGates = inputs[GATE1_INPUT].getPolyVoltage(i) + inputs[GATE2_INPUT].getPolyVoltage(i);
        allGates += gateButton;
        if (clockTrigger[i].process(allGates))
        {
            float delayTime = ((inputs[DELAYCV_INPUT].getPolyVoltage(i)) * delayDepth) + delayKnob;
            delayTime = clamp(delayTime, 0.0f, maxTime);
            delayGates[i].setDelayTimeInSeconds(delayTime);

            float gateWidth = ((inputs[WIDTHCV_INPUT].getPolyVoltage(i)) * widthDepth) + widthKnob;
            gateWidth = clamp(gateWidth, 0.0001f, maxTime);
            delayGates[i].setGateTimeInSeconds(gateWidth);

            delayGates[i].trigger();
        }

        gateOuts[i] = boolToGate(delayGates[i].process());
        outputs[DELAY_OUTPUT].setVoltage(gateOuts[i], i);
    }

    lights[DELAY_LIGHT].setBrightnessSmooth(outputs[DELAY_OUTPUT].getVoltage(0), args.sampleTime * 4.0f);
    
}

struct GateDelayWidget : HCVModuleWidget { GateDelayWidget(GateDelay *module); };

GateDelayWidget::GateDelayWidget(GateDelay *module)
{
    setSkinPath("res/GateDelay.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    int jackX = 49;
    createInputPort(21, 248, GateDelay::GATE1_INPUT);
    createInputPort(76, 248, GateDelay::GATE2_INPUT);
    int knobY = 90;
    createParamComboVertical(15, knobY, GateDelay::DELAY_PARAM, GateDelay::DELAYCV_PARAM, GateDelay::DELAYCV_INPUT);
    createParamComboVertical(70, knobY, GateDelay::WIDTH_PARAM, GateDelay::WIDTHCV_PARAM, GateDelay::WIDTHCV_INPUT);
    
    createHCVButtonSmall(53.5, 251, GateDelay::GATEBUTTON_PARAM);

    createHCVRedLight(75, 320, GateDelay::DELAY_LIGHT);
    createOutputPort(jackX, 310, GateDelay::DELAY_OUTPUT);
    
}

Model *modelGateDelay = createModel<GateDelay, GateDelayWidget>("GateDelay");
