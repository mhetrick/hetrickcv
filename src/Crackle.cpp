#include "HetrickCV.hpp"

struct Crackle : Module 
{
	enum ParamIds 
	{
        RATE_PARAM,
        BROKEN_PARAM,
		NUM_PARAMS
	};
	enum InputIds 
	{
		RATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds 
	{
		DUST_OUTPUT,
		NUM_OUTPUTS
	};

	float lastDensity = 1.0;
	float densityScaled = 1.0;
    float y1 = 0.2643;
    float y2 = 0.0;

	Crackle() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) 
	{
		y1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	}

	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Crackle::step() 
{
	const float densityInput = params[RATE_PARAM].value + inputs[RATE_INPUT].value;
	
	if(lastDensity != densityInput)
	{
		densityScaled = clampf(densityInput, 0.0f, 2.0f)/2.0f;
		densityScaled = powf(densityScaled, 3.0f) + 1.0f;
		lastDensity = densityInput;
    }
    
    const bool brokenMode = (params[BROKEN_PARAM].value > 0.0);
    
    if(brokenMode)
    {
        const float y0 = fabs(y1 * densityScaled - y2 - 0.05f);
        y2 = y1;
        y1 = y0;
        outputs[DUST_OUTPUT].value = clampf(y0, -5.0, 5.0);
    }
    else
    {
        const float y0 = fabs(y1 * densityScaled - y2 - 0.05f);
        y2 = y1;
        y1 = y0;
        outputs[DUST_OUTPUT].value = clampf(y0, -5.0, 5.0);
    }

}


CrackleWidget::CrackleWidget() 
{
	auto *module = new Crackle();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Crackle.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

    //////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, Crackle::RATE_PARAM, 0.0, 2.0, 0.0));
    addParam(createParam<CKSS>(Vec(37, 220), module, Crackle::BROKEN_PARAM, 0.0, 1.0, 0.0));

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 146), module, Crackle::RATE_INPUT));

    //////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Crackle::DUST_OUTPUT));
}
