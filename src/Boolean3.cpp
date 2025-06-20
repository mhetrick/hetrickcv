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

    // Arrays for polyphonic support
    HysteresisGate ins[16][3];
    bool inA[16] = {};
    bool inB[16] = {};
    bool inC[16] = {};
    float outs[16][6] = {};

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
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        inA[c] = ins[c][0].process(inputs[INA_INPUT].getPolyVoltage(c));
        inB[c] = ins[c][1].process(inputs[INB_INPUT].getPolyVoltage(c));
        inC[c] = ins[c][2].process(inputs[INC_INPUT].getPolyVoltage(c));

        if(inputs[INC_INPUT].isConnected())
        {
            // 3-input logic operations
            outs[c][0] = ((inA[c] || inB[c]) || inC[c]) ? HCV_GATE_MAG : 0.0f;  // OR
            outs[c][1] = ((inA[c] && inB[c]) && inC[c]) ? HCV_GATE_MAG : 0.0f;  // AND
            outs[c][2] = (!inA[c] && (inB[c] ^ inC[c])) || (inA[c] && !(inB[c] || inC[c])) ? HCV_GATE_MAG : 0.0f;  // XOR
            outs[c][3] = HCV_GATE_MAG - outs[c][0];  // NOR
            outs[c][4] = HCV_GATE_MAG - outs[c][1];  // NAND
            outs[c][5] = HCV_GATE_MAG - outs[c][2];  // XNOR
        }
        else
        {
            // 2-input logic operations (when C is not connected)
            outs[c][0] = (inA[c] || inB[c]) ? HCV_GATE_MAG : 0.0f;  // OR
            outs[c][1] = (inA[c] && inB[c]) ? HCV_GATE_MAG : 0.0f;  // AND
            outs[c][2] = (inA[c] != inB[c]) ? HCV_GATE_MAG : 0.0f;  // XOR
            outs[c][3] = HCV_GATE_MAG - outs[c][0];  // NOR
            outs[c][4] = HCV_GATE_MAG - outs[c][1];  // NAND
            outs[c][5] = HCV_GATE_MAG - outs[c][2];  // XNOR
        }

        outputs[OR_OUTPUT].setVoltage(outs[c][0], c);
        outputs[AND_OUTPUT].setVoltage(outs[c][1], c);
        outputs[XOR_OUTPUT].setVoltage(outs[c][2], c);
        outputs[NOR_OUTPUT].setVoltage(outs[c][3], c);
        outputs[NAND_OUTPUT].setVoltage(outs[c][4], c);
        outputs[XNOR_OUTPUT].setVoltage(outs[c][5], c);
    }

    // Lights show the state of channel 0
    lights[INA_LIGHT].value = inA[0] ? HCV_GATE_MAG : 0.0f;
    lights[INB_LIGHT].value = inB[0] ? HCV_GATE_MAG : 0.0f;
    lights[INC_LIGHT].value = inC[0] ? HCV_GATE_MAG : 0.0f;

    lights[OR_LIGHT].value = outs[0][0];
    lights[AND_LIGHT].value = outs[0][1];
    lights[XOR_LIGHT].value = outs[0][2];
    lights[NOR_LIGHT].value = outs[0][3];
    lights[NAND_LIGHT].value = outs[0][4];
    lights[XNOR_LIGHT].value = outs[0][5];
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
