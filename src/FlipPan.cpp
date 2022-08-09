#include "HetrickCV.hpp"

struct FlipPan : HCVModule
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

	FlipPan()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(FlipPan::AMOUNT_PARAM, 0.0, 5.0, 2.5, "Pan");
		configParam(FlipPan::SCALE_PARAM, -1.0, 1.0, 1.0, "Pan CV Depth");
        configSwitch(FlipPan::STYLE_PARAM, 0.0, 1.0, 0.0, "Panning Style", {"Linear", "Equal"});

        configBypass(LEFT_INPUT, LEFT_OUTPUT);
        configBypass(RIGHT_INPUT, RIGHT_OUTPUT);
	}

    void process(const ProcessArgs &args) override;

    template <typename T>
    T paraPanShape(const T _input) const
    {
        return (4.0f - _input) * _input * 0.3333333f;
    }

	simd::float_4   insL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, insR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outsL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, outsR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    pansL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, pansR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    pans[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};


void FlipPan::process(const ProcessArgs &args)
{
    int channels = getMaxInputPolyphony();
    outputs[LEFT_OUTPUT].setChannels(channels);
    outputs[RIGHT_OUTPUT].setChannels(channels);

    auto panAmount = params[AMOUNT_PARAM].getValue();
    auto panScale = params[SCALE_PARAM].getValue();

    bool linear = params[STYLE_PARAM].getValue() == 0.0f;

	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c / 4;
		insL[vectorIndex] = simd::float_4::load(inputs[LEFT_INPUT].getVoltages(c));
        insR[vectorIndex] = simd::float_4::load(inputs[RIGHT_INPUT].getVoltages(c));

        pans[vectorIndex] = simd::float_4::load(inputs[AMOUNT_INPUT].getVoltages(c));
        pans[vectorIndex] = (pans[vectorIndex] * panScale) + panAmount;
        pans[vectorIndex] = clamp(pans[vectorIndex], 0.0f, 5.0f) * 0.2f;

        if(linear)
        {
            outsL[vectorIndex] = SIMDLERP(pans[vectorIndex], insR[vectorIndex], insL[vectorIndex]);
            outsR[vectorIndex] = SIMDLERP(pans[vectorIndex], insL[vectorIndex], insR[vectorIndex]);
        }
        else
        {
            pans[vectorIndex] = (pans[vectorIndex] * 2.0f) - 1.0f;
            pansL[vectorIndex] = paraPanShape(1.0f - pans[vectorIndex]);
            pansR[vectorIndex] = paraPanShape(1.0f + pans[vectorIndex]);

            outsL[vectorIndex] = ((insL[vectorIndex] * pansL[vectorIndex]) + (insR[vectorIndex] * pansR[vectorIndex]));
            outsR[vectorIndex] = ((insL[vectorIndex] * pansR[vectorIndex]) + (insR[vectorIndex] * pansL[vectorIndex]));
        }

        outsL[vectorIndex].store(outputs[LEFT_OUTPUT].getVoltages(c));
        outsR[vectorIndex].store(outputs[RIGHT_OUTPUT].getVoltages(c));
	}
}


struct FlipPanWidget : HCVModuleWidget { FlipPanWidget(FlipPan *module); };

FlipPanWidget::FlipPanWidget(FlipPan *module)
{
	setSkinPath("res/FlipPan.svg");
	initializeWidget(module);

	//////PARAMS//////
    createHCVKnob(27, 62, FlipPan::AMOUNT_PARAM);
    createHCVTrimpot(36, 112, FlipPan::SCALE_PARAM);
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
