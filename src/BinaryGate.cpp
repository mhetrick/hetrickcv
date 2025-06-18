#include "HetrickCV.hpp"

struct BinaryGate : HCVModule
{
    enum ParamIds
    {
        ON_PARAM,
        OFF_PARAM,
        TOGGLE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        ON_INPUT,
        OFF_INPUT,
        TOGGLE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        GATE_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        GATE_LIGHT,
        NUM_LIGHTS
	};

	BinaryGate()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configButton(ON_PARAM, "On Button");
        configButton(OFF_PARAM, "Off Button");
        configButton(TOGGLE_PARAM, "Toggle Button");

        configInput(ON_INPUT, "On Gate");
        configInput(OFF_INPUT, "Off Gate");
        configInput(TOGGLE_INPUT, "Toggle Gate");
        
        configOutput(GATE_OUTPUT, "Main Gate");
	}

	void process(const ProcessArgs &args) override;

    bool gateState[16] = {};
    dsp::SchmittTrigger onTrigger[16], offTrigger[16], toggleTrigger[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void BinaryGate::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        // Get input voltages for this channel (0V if channel doesn't exist)
        float onVoltage = inputs[ON_INPUT].getPolyVoltage(c);
        float offVoltage = inputs[OFF_INPUT].getPolyVoltage(c);
        float toggleVoltage = inputs[TOGGLE_INPUT].getPolyVoltage(c);

        // For buttons, only apply to channel 0 (monophonic control)
        float onButtonVoltage = (c == 0) ? params[ON_PARAM].getValue() : 0.0f;
        float offButtonVoltage = (c == 0) ? params[OFF_PARAM].getValue() : 0.0f;
        float toggleButtonVoltage = (c == 0) ? params[TOGGLE_PARAM].getValue() : 0.0f;

        if(onTrigger[c].process(onVoltage + onButtonVoltage))
        {
            gateState[c] = true;
        }

        if(offTrigger[c].process(offVoltage + offButtonVoltage))
        {
            gateState[c] = false;
        }

        if(toggleTrigger[c].process(toggleVoltage + toggleButtonVoltage))
        {
            gateState[c] = !gateState[c];
        }

        // Set output voltage for this channel
        outputs[GATE_OUTPUT].setVoltage(gateState[c] ? HCV_GATE_MAG : 0.0f, c);
    }

    // Light shows the state of channel 0
    lights[GATE_LIGHT].setBrightness(gateState[0] ? 1.0f : 0.0f);
}

struct BinaryGateWidget : HCVModuleWidget { BinaryGateWidget(BinaryGate *module); };

BinaryGateWidget::BinaryGateWidget(BinaryGate *module)
{
    setSkinPath("res/BinaryGate.svg");
    initializeWidget(module, true);

    //////PARAMS//////

    //////INPUTS//////
    const float inSpacing = 67.0f;
    const float jackX = 17.5f;

    for(int i = 0; i < BinaryGate::NUM_INPUTS; i++)
    {
        createInputPort(jackX, 78 + (i*inSpacing), BinaryGate::ON_INPUT + i);
        createHCVButtonSmall(jackX + 4.5f, 107 + (i*inSpacing), BinaryGate::ON_PARAM + i);
    }

    //////OUTPUTS//////
    const int outputY = 282;
    createOutputPort(jackX, outputY, BinaryGate::GATE_OUTPUT);
    createHCVRedLightForJack(jackX, outputY, BinaryGate::GATE_LIGHT);
}

Model *modelBinaryGate = createModel<BinaryGate, BinaryGateWidget>("BinaryGate");
