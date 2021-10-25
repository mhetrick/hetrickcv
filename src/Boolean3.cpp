#include "HetrickCV.hpp"

struct Boolean3 : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        INA_INPUT,
        INB_INPUT,
        INC_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OR_OUTPUT,
        AND_OUTPUT,
        XOR_OUTPUT,
        NOR_OUTPUT,
        NAND_OUTPUT,
        XNOR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        OR_LIGHT,
        AND_LIGHT,
        XOR_LIGHT,
        NOR_LIGHT,
        NAND_LIGHT,
        XNOR_LIGHT,
		INA_LIGHT,
        INB_LIGHT,
        INC_LIGHT,
        NUM_LIGHTS
	};

    HysteresisGate ins[3];
    bool inA = false;
    bool inB = false;
    bool inC = false;
    float outs[6] = {};

	Boolean3()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configInput(INA_INPUT, "A");
        configInput(INB_INPUT, "B");
        configInput(INC_INPUT, "C");

        configOutput(OR_OUTPUT, "OR");
        configOutput(AND_OUTPUT, "AND");
        configOutput(XOR_OUTPUT, "XOR");
        configOutput(NOR_OUTPUT, "NOR");
        configOutput(NAND_OUTPUT, "NAND");
        configOutput(XNOR_OUTPUT, "XNOR");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Boolean3::process(const ProcessArgs &args)
{
    inA = ins[0].process(inputs[INA_INPUT].getVoltage());
    inB = ins[1].process(inputs[INB_INPUT].getVoltage());
    inC = ins[2].process(inputs[INC_INPUT].getVoltage());

    lights[INA_LIGHT].value = inA ? 5.0f : 0.0f;
    lights[INB_LIGHT].value = inB ? 5.0f : 0.0f;
    lights[INC_LIGHT].value = inC ? 5.0f : 0.0f;

    if(inputs[INC_INPUT].isConnected())
    {
        outs[0] = ((inA || inB) || inC) ? 5.0f : 0.0f;
        outs[1] = ((inA && inB) && inC) ? 5.0f : 0.0f;
        outs[2] = (!inA && (inB ^ inC)) || (inA && !(inB || inC)) ? 5.0f : 0.0f;
        outs[3] = 5.0f - outs[0];
        outs[4] = 5.0f - outs[1];
        outs[5] = 5.0f - outs[2];
    }
    else
    {
        outs[0] = (inA || inB) ? 5.0f : 0.0f;
        outs[1] = (inA && inB) ? 5.0f : 0.0f;
        outs[2] = (inA != inB) ? 5.0f : 0.0f;
        outs[3] = 5.0f - outs[0];
        outs[4] = 5.0f - outs[1];
        outs[5] = 5.0f - outs[2];
    }


    outputs[OR_OUTPUT].setVoltage(outs[0]);
    outputs[AND_OUTPUT].setVoltage(outs[1]);
    outputs[XOR_OUTPUT].setVoltage(outs[2]);
    outputs[NOR_OUTPUT].setVoltage(outs[3]);
    outputs[NAND_OUTPUT].setVoltage(outs[4]);
    outputs[XNOR_OUTPUT].setVoltage(outs[5]);

    lights[OR_LIGHT].value = outs[0];
    lights[AND_LIGHT].value = outs[1];
    lights[XOR_LIGHT].value = outs[2];
    lights[NOR_LIGHT].value = outs[3];
    lights[NAND_LIGHT].value = outs[4];
    lights[XNOR_LIGHT].value = outs[5];
}

struct Boolean3Widget : HCVModuleWidget { Boolean3Widget(Boolean3 *module); };

Boolean3Widget::Boolean3Widget(Boolean3 *module)
{
    setSkinPath("res/Boolean3.svg");
    initializeWidget(module);

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 105), module, Boolean3::INA_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 195), module, Boolean3::INB_INPUT));
    addInput(createInput<PJ301MPort>(Vec(10, 285), module, Boolean3::INC_INPUT));
    addChild(createLight<SmallLight<RedLight>>(Vec(18, 92), module, Boolean3::INA_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(18, 182), module, Boolean3::INB_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(18, 272), module, Boolean3::INC_LIGHT));

    //////OUTPUTS//////
    for(int i = 0; i < 6; i++)
    {
        const int yPos = i*45;
        addOutput(createOutput<PJ301MPort>(Vec(45, 60 + yPos), module, Boolean3::OR_OUTPUT + i));
        addChild(createLight<SmallLight<RedLight>>(Vec(74, 68 + yPos), module, Boolean3::OR_LIGHT + i));
    }

}

Model *modelBoolean3 = createModel<Boolean3, Boolean3Widget>("Boolean3");
