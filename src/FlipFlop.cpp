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

    SchmittTrigger clockTrigger;
    float outs[4] = {};
    bool toggle = false;
    bool dataIn = false;

	FlipFlop() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) 
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
    lights[DATA_LIGHT].value = dataIn ? 5.0f : 0.0f;
    lights[TOGGLE_LIGHT].value = (inputs[INT_INPUT].value >= 1.0f) ? 5.0f : 0.0f;

    if (clockTrigger.process(inputs[INT_INPUT].value))
    {
        toggle = !toggle;

        outs[0] = toggle ? 5.0f : 0.0f;
        outs[1] = lights[DATA_LIGHT].value;

        outs[2] = 5.0f - outs[0];
        outs[3] = 5.0f - outs[1];
    }

    outputs[FFT_OUTPUT].value = outs[0];
    outputs[FFD_OUTPUT].value = outs[1];
    outputs[FFTNOT_OUTPUT].value = outs[2];
    outputs[FFDNOT_OUTPUT].value = outs[3];

    lights[FFT_LIGHT].value = outs[0];
    lights[FFD_LIGHT].value = outs[1];
    lights[FFTNOT_LIGHT].value = outs[2];
    lights[FFDNOT_LIGHT].value = outs[3];
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
    addChild(createLight<SmallLight<RedLight>>(Vec(18, 87), module, FlipFlop::TOGGLE_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(63, 87), module, FlipFlop::DATA_LIGHT));

    for(int i = 0; i < 4; i++)
    {
        const int yPos = i*45;
        addOutput(createOutput<PJ301MPort>(Vec(33, 150 + yPos), module, FlipFlop::FFT_OUTPUT + i));
        addChild(createLight<SmallLight<RedLight>>(Vec(70, 158 + yPos), module, FlipFlop::FFT_LIGHT + i));
    }
}
