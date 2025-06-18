#include "HetrickCV.hpp"

/*
│
│ analog shift register
└───────►┐
         │
         │
         └───────►┐
                  │
                  │
                  └───────►┐
                           │
                           │
                           └────────►
 */

struct ASR : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        CLK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        STAGE1_OUTPUT,
        STAGE2_OUTPUT,
        STAGE3_OUTPUT,
        STAGE4_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        OUT1_POS_LIGHT, OUT1_NEG_LIGHT,
		OUT2_POS_LIGHT, OUT2_NEG_LIGHT,
		OUT3_POS_LIGHT, OUT3_NEG_LIGHT,
		OUT4_POS_LIGHT, OUT4_NEG_LIGHT,
        NUM_LIGHTS
	};

    // Arrays for polyphonic support
    dsp::SchmittTrigger clockTrigger[16];
    float stages[16][4] = {};

	ASR()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(MAIN_INPUT, "Data");
        configInput(CLK_INPUT, "Clock");
        configOutput(STAGE1_OUTPUT, "Register Stage 1");
        configOutput(STAGE2_OUTPUT, "Register Stage 2");
        configOutput(STAGE3_OUTPUT, "Register Stage 3");
        configOutput(STAGE4_OUTPUT, "Register Stage 4");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void ASR::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        if (clockTrigger[c].process(inputs[CLK_INPUT].getPolyVoltage(c)))
        {
            stages[c][3] = stages[c][2];
            stages[c][2] = stages[c][1];
            stages[c][1] = stages[c][0];
            stages[c][0] = inputs[MAIN_INPUT].getPolyVoltage(c);
        }

        outputs[STAGE1_OUTPUT].setVoltage(stages[c][0], c);
        outputs[STAGE2_OUTPUT].setVoltage(stages[c][1], c);
        outputs[STAGE3_OUTPUT].setVoltage(stages[c][2], c);
        outputs[STAGE4_OUTPUT].setVoltage(stages[c][3], c);
    }

    // Lights show the state of channel 0
    lights[OUT1_POS_LIGHT].setSmoothBrightness(fmaxf(0.0, stages[0][0] / 5.0), 10);
    lights[OUT1_NEG_LIGHT].setSmoothBrightness(fmaxf(0.0, -stages[0][0] / 5.0), 10);

    lights[OUT2_POS_LIGHT].setSmoothBrightness(fmaxf(0.0, stages[0][1] / 5.0), 10);
    lights[OUT2_NEG_LIGHT].setSmoothBrightness(fmaxf(0.0, -stages[0][1] / 5.0), 10);

    lights[OUT3_POS_LIGHT].setSmoothBrightness(fmaxf(0.0, stages[0][2] / 5.0), 10);
    lights[OUT3_NEG_LIGHT].setSmoothBrightness(fmaxf(0.0, -stages[0][2] / 5.0), 10);

    lights[OUT4_POS_LIGHT].setSmoothBrightness(fmaxf(0.0, stages[0][3] / 5.0), 10);
    lights[OUT4_NEG_LIGHT].setSmoothBrightness(fmaxf(0.0, -stages[0][3] / 5.0), 10);
}

struct ASRWidget : HCVModuleWidget { ASRWidget(ASR *module); };

ASRWidget::ASRWidget(ASR *module)
{
    setSkinPath("res/ASR.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 100), module, ASR::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(55, 100), module, ASR::CLK_INPUT));

    for(int i = 0; i < 4; i++)
    {
        const int yPos = i*45;
        addOutput(createOutput<PJ301MPort>(Vec(33, 150 + yPos), module, ASR::STAGE1_OUTPUT + i));
        addChild(createLight<SmallLight<GreenRedLight>>(Vec(70, 158 + yPos), module, ASR::OUT1_POS_LIGHT + i*2));
    }
}

Model *modelASR = createModel<ASR, ASRWidget>("ASR");
