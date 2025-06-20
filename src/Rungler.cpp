#include "HetrickCV.hpp"
#include "DSP/HCVShiftRegister.h"

struct Rungler : HCVModule
{
	enum ParamIds
	{
        COMPARE_PARAM, COMPARE_DEPTH_PARAM,
        SCALE_PARAM, SCALE_DEPTH_PARAM,
        WRITE_PARAM,
        XOR_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        DATA_INPUT,
        COMPARE_INPUT,
        SCALE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
        SEQ_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
	{
        CLOCK_LIGHT,

        OUT1_LIGHT,
        OUT2_LIGHT,
        OUT3_LIGHT,
        OUT4_LIGHT,
        OUT5_LIGHT,
        OUT6_LIGHT,
        OUT7_LIGHT,
        OUT8_LIGHT,

		NUM_LIGHTS
	};

	Rungler()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(Rungler::COMPARE_PARAM, -5.0, 5.0, 0.0, "Comparator Voltage");
		configParam(Rungler::COMPARE_DEPTH_PARAM, -1.0, 1.0, 0.0, "Compare CV Depth");

        configParam(Rungler::SCALE_PARAM, -5.0, 5.0, 5.0, "Output Scale");
		configParam(Rungler::SCALE_DEPTH_PARAM, -1.0, 1.0, 0.0, "Output Scale CV Depth");
        
        configSwitch(WRITE_PARAM, 0.0, 1.0, 1.0, "Write Enable", {"Loop", "Write"});
        configSwitch(XOR_PARAM, 0.0, 1.0, 1.0, "Feedback Mode", {"Direct", "XOR"});

        configInput(CLOCK_INPUT, "Clock");
        configInput(DATA_INPUT, "Data");
        configInput(COMPARE_INPUT, "Compare CV");
        configInput(SCALE_INPUT, "Scale CV");

        configOutput(SEQ_OUTPUT, "Sequence");
        for (int i = 0; i < 8; i++)
        {
            configOutput(OUT1_OUTPUT + i, "Stage " + std::to_string(i + 1));
        }
    }

    void process(const ProcessArgs &args) override;

    // Arrays for polyphonic support
    HCVRungler rungler[16];
    dsp::SchmittTrigger clockTrigger[16];
    float runglerOut[16] = {};

    void onReset() override
    {
        for (int c = 0; c < 16; c++)
        {
            rungler[c].emptyRegister();
        }
    }

    void onRandomize() override
    {

	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void Rungler::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Global mode settings (front-panel switches)
    const bool writeMode = (params[WRITE_PARAM].getValue() > 0.0f);
    const bool xorMode = (params[XOR_PARAM].getValue() > 0.0f);

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        if (clockTrigger[c].process(inputs[CLOCK_INPUT].getPolyVoltage(c)))
        {
            rungler[c].enableXORFeedback(xorMode);

            if(!writeMode)
            {
                rungler[c].advanceRegisterFrozen();
            }
            else
            {
                double compare = params[COMPARE_PARAM].getValue() + (params[COMPARE_DEPTH_PARAM].getValue() * inputs[COMPARE_INPUT].getPolyVoltage(c));
                compare = clamp(compare, -5.0f, 5.0f);

                rungler[c].advanceRegister(inputs[DATA_INPUT].getPolyVoltage(c) > compare);
            }

            runglerOut[c] = rungler[c].getRunglerOut();
        }

        double scale = params[SCALE_PARAM].getValue() + (params[SCALE_DEPTH_PARAM].getValue() * inputs[SCALE_INPUT].getPolyVoltage(c));
        scale = clamp(scale, -5.0f, 5.0f);

        outputs[SEQ_OUTPUT].setVoltage(runglerOut[c] * scale, c);

        // Set individual stage outputs for each channel
        for(int i = 0; i < 8; i++)
        {
            outputs[OUT1_OUTPUT + i].setVoltage(rungler[c].dataRegister[i] ? 5.0f : 0.0f, c);
        }
    }

    // Lights show the state of channel 0
    for(int i = 0; i < 8; i++)
    {
        lights[OUT1_LIGHT + i].setSmoothBrightness(outputs[OUT1_OUTPUT + i].getVoltage(0) * 0.2f, args.sampleTime);
    }
}


struct RunglerWidget : HCVModuleWidget { RunglerWidget(Rungler *module); };

RunglerWidget::RunglerWidget(Rungler *module)
{
    setSkinPath("res/Rungler.svg");
    initializeWidget(module);
    
    //PARAMS
    const float knobY = 64.0f;
    const float knobX = 12.0f;
    const float rightOffset = 57.0f;

    createParamComboVertical(knobX, knobY, Rungler::COMPARE_PARAM, Rungler::COMPARE_DEPTH_PARAM, Rungler::COMPARE_INPUT);
    createParamComboVertical(knobX + rightOffset, knobY, Rungler::SCALE_PARAM, Rungler::SCALE_DEPTH_PARAM, Rungler::SCALE_INPUT);


    //
    const float inputY = 230.0f;
    const float dataX = 75.0f;
    const float clockX = 16.0f;
    const float outY = 320;

    createInputPort(clockX, inputY, Rungler::CLOCK_INPUT);
    createInputPort(dataX, inputY, Rungler::DATA_INPUT);

    const float switchX = 7.0f;
    createHCVSwitchVert(switchX, outY, Rungler::WRITE_PARAM);
    createHCVSwitchVert(switchX + 29, outY, Rungler::XOR_PARAM);

    createOutputPort(dataX, outY, Rungler::SEQ_OUTPUT);

    //////BLINKENLIGHTS//////
    const int outXPos = 145;
    const int outLightX = 120;
    for(int i = 0; i < 8; i++)
    {
        const int yPos = 50 + (40 * i);
        const int lightY = 59 + (40 * i);

        //////OUTPUTS//////
        createOutputPort(outXPos, yPos, Rungler::OUT1_OUTPUT + i);

        //////BLINKENLIGHTS//////
        addChild(createLight<SmallLight<RedLight>>(Vec(outLightX, lightY), module, Rungler::OUT1_LIGHT + i));
    }
}

Model *modelRungler = createModel<Rungler, RunglerWidget>("Rungler");
