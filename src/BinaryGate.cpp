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

    bool gateState = false;
    dsp::SchmittTrigger onTrigger, offTrigger, toggleTrigger;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void BinaryGate::process(const ProcessArgs &args)
{
    if(onTrigger.process(inputs[ON_INPUT].getVoltage() + params[ON_PARAM].getValue()))
    {
        gateState = true;
    }

    if(offTrigger.process(inputs[OFF_INPUT].getVoltage() + params[OFF_PARAM].getValue()))
    {
        gateState = false;
    }

    if(toggleTrigger.process(inputs[TOGGLE_INPUT].getVoltage() + params[TOGGLE_PARAM].getValue()))
    {
        gateState = !gateState;
    }

    outputs[GATE_OUTPUT].setVoltage(gateState ? 5.0 : 0.0);

    lights[GATE_LIGHT].setBrightness(gateState ? 1.0 : 0.0);
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
        addInput(createInput<PJ301MPort>(Vec(jackX, 78 + (i*inSpacing)), module, BinaryGate::ON_INPUT + i));
        addParam(createParam<TL1105>(Vec(jackX + 4.5, 107 + (i*inSpacing)), module, BinaryGate::ON_PARAM + i));
    }

    //////OUTPUTS//////
    const int outputY = 282;
    addOutput(createOutput<PJ301MPort>(Vec(jackX - 6.0f, outputY), module, BinaryGate::GATE_OUTPUT));
    addChild(createLight<SmallLight<RedLight>>(Vec(jackX + 22.0f, outputY + 10.0f), module, BinaryGate::GATE_LIGHT));
}

Model *modelBinaryGate = createModel<BinaryGate, BinaryGateWidget>("BinaryGate");
