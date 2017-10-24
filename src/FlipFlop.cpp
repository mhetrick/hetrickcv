#include "HetrickCV.hpp"
#include "dsp/digital.hpp"

struct FlipFlop : Module 
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

    SchmittTrigger clockTrigger;
    float outs[4] = {};
    bool toggle = false;
    bool dataIn = false;
    float dataLight = 0.0f;
    float toggleLight = 0.0f;

	FlipFlop() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) 
	{
		
	}

	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void FlipFlop::step() 
{
    dataIn = (inputs[IND_INPUT].value >= 1.0f);
    dataLight = dataIn ? 5.0f : 0.0f;
    toggleLight = (inputs[INT_INPUT].value >= 1.0f) ? 5.0f : 0.0f;

    if (clockTrigger.process(inputs[INT_INPUT].value))
    {
        toggle = !toggle;

        outs[0] = toggle ? 5.0f : 0.0f;
        outs[1] = dataLight;

        outs[2] = 5.0f - outs[0];
        outs[3] = 5.0f - outs[1];
    }

    outputs[FFT_OUTPUT].value = outs[0];
    outputs[FFD_OUTPUT].value = outs[1];
    outputs[FFTNOT_OUTPUT].value = outs[2];
    outputs[FFDNOT_OUTPUT].value = outs[3];
}


FlipFlopWidget::FlipFlopWidget() 
{
	auto *module = new FlipFlop();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/FlipFlop.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 100), module, FlipFlop::INT_INPUT));
    addInput(createInput<PJ301MPort>(Vec(55, 100), module, FlipFlop::IND_INPUT));

    //////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(33, 150), module, FlipFlop::FFT_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(33, 195), module, FlipFlop::FFD_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(33, 240), module, FlipFlop::FFTNOT_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, FlipFlop::FFDNOT_OUTPUT));

    //////BLINKENLIGHTS//////
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(70, 158), &module->outs[0]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(70, 203), &module->outs[1]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(70, 248), &module->outs[2]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(70, 293), &module->outs[3]));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(18, 87), &module->toggleLight));
    addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(63, 87), &module->dataLight));
}
