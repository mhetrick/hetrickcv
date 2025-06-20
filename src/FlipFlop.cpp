#include "HetrickCV.hpp"

struct FlipFlop : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        INT_INPUT,
        IND_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        FFT_OUTPUT,
        FFD_OUTPUT,
        FFTNOT_OUTPUT,
        FFDNOT_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        FFT_LIGHT,
        FFD_LIGHT,
        FFTNOT_LIGHT,
        FFDNOT_LIGHT,
		TOGGLE_LIGHT,
        DATA_LIGHT,
        NUM_LIGHTS
    };

    // Arrays for polyphonic support
    dsp::SchmittTrigger clockTrigger[16];
    float outs[16][4] = {};
    bool toggle[16] = {};
    bool dataIn[16] = {};

    FlipFlop()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        configInput(INT_INPUT, "Clock");
        configInput(IND_INPUT, "Data");
        
        configOutput(FFT_OUTPUT, "Toggle");
        configOutput(FFD_OUTPUT, "Data");
        configOutput(FFTNOT_OUTPUT, "Toggle Inverted");
        configOutput(FFDNOT_OUTPUT, "Data Inverted");
        
        onReset();
    }

    void process(const ProcessArgs &args) override;

    void onReset() override
    {
        for (int c = 0; c < 16; c++)
        {
            toggle[c] = false;
            dataIn[c] = false;
            outs[c][0] = 0.0f;
            outs[c][1] = 0.0f;
            outs[c][2] = HCV_GATE_MAG;
            outs[c][3] = HCV_GATE_MAG;
        }
    }

    // For more advanced Module features, read Rack's engine.hpp header file
    // - dataToJson, dataFromJson: serialization of internal data
    // - onSampleRateChange: event triggered by a change of sample rate
    // - reset, randomize: implements special behavior when user clicks these from the context menu
};

void FlipFlop::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        dataIn[c] = (inputs[IND_INPUT].getPolyVoltage(c) >= 1.0f);

        if (clockTrigger[c].process(inputs[INT_INPUT].getPolyVoltage(c)))
        {
            toggle[c] = !toggle[c];

            outs[c][0] = toggle[c] ? HCV_GATE_MAG : 0.0f;
            outs[c][1] = dataIn[c] ? HCV_GATE_MAG : 0.0f;

            outs[c][2] = HCV_GATE_MAG - outs[c][0];
            outs[c][3] = HCV_GATE_MAG - outs[c][1];
        }

        outputs[FFT_OUTPUT].setVoltage(outs[c][0], c);
        outputs[FFD_OUTPUT].setVoltage(outs[c][1], c);
        outputs[FFTNOT_OUTPUT].setVoltage(outs[c][2], c);
        outputs[FFDNOT_OUTPUT].setVoltage(outs[c][3], c);
    }

    // Lights show the state of channel 0
    lights[DATA_LIGHT].value = dataIn[0] ? HCV_GATE_MAG : 0.0f;
    lights[TOGGLE_LIGHT].value = (inputs[INT_INPUT].getVoltage() >= 1.0f) ? HCV_GATE_MAG : 0.0f;

    lights[FFT_LIGHT].value = outs[0][0];
    lights[FFD_LIGHT].value = outs[0][1];
    lights[FFTNOT_LIGHT].value = outs[0][2];
    lights[FFDNOT_LIGHT].value = outs[0][3];
}

struct FlipFlopWidget : HCVModuleWidget { FlipFlopWidget(FlipFlop *module); };

FlipFlopWidget::FlipFlopWidget(FlipFlop *module)
{
    setSkinPath("res/FlipFlop.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 100), module, FlipFlop::INT_INPUT));
    addInput(createInput<PJ301MPort>(Vec(55, 100), module, FlipFlop::IND_INPUT));
    addChild(createLight<SmallLight<RedLight>>(Vec(18, 87), module, FlipFlop::TOGGLE_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(63, 87), module, FlipFlop::DATA_LIGHT));

    for(int i = 0; i < 4; i++)
    {
        const int yPos = i*45;
        addOutput(createOutput<PJ301MPort>(Vec(33, 150 + yPos), module, FlipFlop::FFT_OUTPUT + i));
        addChild(createLight<SmallLight<RedLight>>(Vec(70, 158 + yPos), module, FlipFlop::FFT_LIGHT + i));
    }
}

Model *modelFlipFlop = createModel<FlipFlop, FlipFlopWidget>("FlipFlop");
