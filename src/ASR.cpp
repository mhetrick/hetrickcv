#include "HetrickCV.hpp"
#include "dsp/digital.hpp"

struct ASR : Module 
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
        STAGE1_LIGHT,
        STAGE2_LIGHT,
		STAGE3_LIGHT,
        STAGE4_LIGHT,
        NUM_LIGHTS
	};

    SchmittTrigger clockTrigger;
    float stages[4] = {};

	ASR() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) 
	{
		
	}

	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void ASR::step() 
{
    if (clockTrigger.process(inputs[CLK_INPUT].value))
    {
        stages[3] = stages[2];
        stages[2] = stages[1];
        stages[1] = stages[0];
        stages[0] = inputs[MAIN_INPUT].value;
    }

    outputs[STAGE1_OUTPUT].value = stages[0];
    outputs[STAGE2_OUTPUT].value = stages[1];
    outputs[STAGE3_OUTPUT].value = stages[2];
    outputs[STAGE4_OUTPUT].value = stages[3];

    lights[STAGE1_LIGHT].value = stages[0];
    lights[STAGE2_LIGHT].value = stages[1];
    lights[STAGE3_LIGHT].value = stages[2];
    lights[STAGE4_LIGHT].value = stages[3];
}


ASRWidget::ASRWidget() 
{
	auto *module = new ASR();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ASR.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 100), module, ASR::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(55, 100), module, ASR::CLK_INPUT));

    //////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(33, 150), module, ASR::STAGE1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(33, 195), module, ASR::STAGE2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(33, 240), module, ASR::STAGE3_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, ASR::STAGE4_OUTPUT));

    //////BLINKENLIGHTS//////
    addChild(createLight<SmallLight<GreenRedLight>>(Vec(70, 158), module, ASR::STAGE1_LIGHT));
    addChild(createLight<SmallLight<GreenRedLight>>(Vec(70, 203), module, ASR::STAGE2_LIGHT));
    addChild(createLight<SmallLight<GreenRedLight>>(Vec(70, 248), module, ASR::STAGE3_LIGHT));
    addChild(createLight<SmallLight<GreenRedLight>>(Vec(70, 293), module, ASR::STAGE4_LIGHT));
}