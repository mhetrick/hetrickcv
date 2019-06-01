#include "HetrickCV.hpp"

struct FlipPan : Module
{
	enum ParamIds
	{
		AMOUNT_PARAM,
        SCALE_PARAM,
        STYLE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        LEFT_INPUT,
        RIGHT_INPUT,
        AMOUNT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
		NUM_OUTPUTS
	};

	FlipPan() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
	{
		configParam(FlipPan::AMOUNT_PARAM, 0.0, 5.0, 2.5, "");
		configParam(FlipPan::SCALE_PARAM, -1.0, 1.0, 1.0, "");
		configParam(FlipPan::STYLE_PARAM, 0.0, 1.0, 0.0, "");
	}

    void process(const ProcessArgs &args) override;

    float paraPanShape(const float _input) const
    {
        return (4.0f - _input) * _input * 0.3333333f;
    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void FlipPan::process(const ProcessArgs &args)
{
	float inL = inputs[LEFT_INPUT].getVoltage();
	float inR = inputs[RIGHT_INPUT].getVoltage();

    bool linear = (params[STYLE_PARAM].getValue() == 0.0f);

    float pan = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
    pan = clamp(pan, 0.0f, 5.0f) * 0.2f;

    if(linear)
    {
        outputs[LEFT_OUTPUT].setVoltage(LERP(pan, inR, inL));
        outputs[RIGHT_OUTPUT].setVoltage(LERP(pan, inL, inR));
    }
    else
    {
        pan = (pan * 2.0f) - 1.0f;
        const float panL = paraPanShape(1.0f - pan);
        const float panR = paraPanShape(1.0f + pan);

        outputs[LEFT_OUTPUT].setVoltage((inL * panL) + (inR * panR));
        outputs[RIGHT_OUTPUT].setVoltage((inL * panR) + (inR * panL));
    }
}

struct CKSSRot : SVGSwitch {
	CKSSRot() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_1.svg")));
	}
};


struct FlipPanWidget : ModuleWidget { FlipPanWidget(FlipPan *module); };

FlipPanWidget::FlipPanWidget(FlipPan *module) : ModuleWidget(module)
{
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		auto *panel = new SvgPanel();
		panel->box.size = box.size;
		panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/FlipPan.svg")));
		addChild(panel);
	}

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

	//////PARAMS//////
	addParam(createParam<Davies1900hBlackKnob>(Vec(27, 62), module, FlipPan::AMOUNT_PARAM));
    addParam(createParam<Trimpot>(Vec(36, 112), module, FlipPan::SCALE_PARAM));
    addParam(createParam<CKSSRot>(Vec(35, 200), module, FlipPan::STYLE_PARAM));

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 235), module, FlipPan::LEFT_INPUT));
    addInput(createInput<PJ301MPort>(Vec(55, 235), module, FlipPan::RIGHT_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, FlipPan::AMOUNT_INPUT));

	//////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(10, 285), module, FlipPan::LEFT_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(55, 285), module, FlipPan::RIGHT_OUTPUT));
}

Model *modelFlipPan = createModel<FlipPan, FlipPanWidget>("FlipPan");
