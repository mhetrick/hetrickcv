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
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};

	float lastDensity = 1.0;
	float densityScaled = 1.0;
    float y1 = 0.2643;
	float y2 = 0.0;

	float lasty1 = 0.2643f;

	Crackle() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
	{
		configParam(Crackle::RATE_PARAM, 0.0, 2.0, 1.7, "");
		configParam(Crackle::BROKEN_PARAM, 0.0, 1.0, 1.0, "");

		y1 = randomf();
		y2 = 0.0f;
		lasty1 = 0.0f;
	}

	void process(const ProcessArgs &args) override;

	void onReset() override
	{
		y1 = randomf();
		y2 = 0.0f;
		lasty1 = 0.0f;
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Crackle::process(const ProcessArgs &args)
{
	const float densityInput = params[RATE_PARAM].getValue() + inputs[RATE_INPUT].getVoltage();

	if(lastDensity != densityInput)
	{
		densityScaled = clampf(densityInput, 0.0f, 2.0f)/2.0f;
		densityScaled = powf(densityScaled, 3.0f) + 1.0f;
		lastDensity = densityInput;
    }

    const bool brokenMode = (params[BROKEN_PARAM].getValue() == 0.0);

    if(brokenMode)
    {
        const float y0 = fabs(y1 * densityScaled - y2 - 0.05f);
		y2 = y1;
		y1 = lasty1;
		lasty1 = clampf(y0, -1.0f, 1.0f);
        outputs[MAIN_OUTPUT].setVoltage(clampf(y0 * 5.0f, -5.0, 5.0));
    }
    else
    {
        const float y0 = fabs(y1 * densityScaled - y2 - 0.05f);
        y2 = y1;
        y1 = y0;
        outputs[MAIN_OUTPUT].setVoltage(clampf(y0 * 5.0f, -5.0, 5.0));
    }

}


struct CrackleWidget : ModuleWidget { CrackleWidget(Crackle *module); };

CrackleWidget::CrackleWidget(Crackle *module) : ModuleWidget(module)
{
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Crackle.svg")));
		addChild(panel);
	}

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    //////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, Crackle::RATE_PARAM));
    addParam(createParam<CKSS>(Vec(37, 220), module, Crackle::BROKEN_PARAM));

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 146), module, Crackle::RATE_INPUT));

    //////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Crackle::MAIN_OUTPUT));
}

Model *modelCrackle = createModel<Crackle, CrackleWidget>("Crackle");
