#include "HetrickCV.hpp"
        

struct MidSide : HCVModule
{
	enum ParamIds
	{
        WIDTH_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        INL_INPUT,
        INR_INPUT,
        INM_INPUT,
        INS_INPUT,
        WIDTH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUTM_OUTPUT,
        OUTS_OUTPUT,
        OUTL_OUTPUT,
        OUTR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
	};

	MidSide()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MidSide::WIDTH_PARAM, -5.0, 5.0, 0.0, "Width");
	}

	void process(const ProcessArgs &args) override;

    simd::float_4   insL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, insR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    insM[4] = {0.0f, 0.0f, 0.0f, 0.0f}, insS[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outsL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, outsR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outsM[4] = {0.0f, 0.0f, 0.0f, 0.0f}, outsS[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    widths[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void MidSide::process(const ProcessArgs &args)
{
    int channels = getMaxInputPolyphony();
    outputs[OUTL_OUTPUT].setChannels(channels);
    outputs[OUTR_OUTPUT].setChannels(channels);
    outputs[OUTM_OUTPUT].setChannels(channels);
    outputs[OUTS_OUTPUT].setChannels(channels);

    auto widthAmount = params[WIDTH_PARAM].getValue();

    bool midConnected = inputs[INM_INPUT].isConnected();
    bool sideConnected = inputs[INS_INPUT].isConnected();

	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c / 4;
		insL[vectorIndex] = simd::float_4::load(inputs[INL_INPUT].getVoltages(c));
        insR[vectorIndex] = simd::float_4::load(inputs[INR_INPUT].getVoltages(c));

        widths[vectorIndex] = simd::float_4::load(inputs[WIDTH_INPUT].getVoltages(c)) + widthAmount;
        widths[vectorIndex] = clamp((widths[vectorIndex] * 0.1f) + 0.5f, 0.0f, 1.0f);

        outsM[vectorIndex] = (insL[vectorIndex] + insR[vectorIndex]) * 0.5f;
        outsS[vectorIndex] = (insR[vectorIndex] - insL[vectorIndex]) * widths[vectorIndex];

        insM[vectorIndex] = midConnected ? simd::float_4::load(inputs[INM_INPUT].getVoltages(c)) : outsM[vectorIndex];
        insS[vectorIndex] = sideConnected ? simd::float_4::load(inputs[INS_INPUT].getVoltages(c)) : outsS[vectorIndex];

        outsL[vectorIndex] = insM[vectorIndex] - insS[vectorIndex];
        outsR[vectorIndex] = insM[vectorIndex] + insS[vectorIndex];

        outsL[vectorIndex].store(outputs[OUTL_OUTPUT].getVoltages(c));
        outsR[vectorIndex].store(outputs[OUTR_OUTPUT].getVoltages(c));
        outsM[vectorIndex].store(outputs[OUTM_OUTPUT].getVoltages(c));
        outsS[vectorIndex].store(outputs[OUTS_OUTPUT].getVoltages(c));
	}
}

struct MidSideWidget : HCVModuleWidget { MidSideWidget(MidSide *module); };

MidSideWidget::MidSideWidget(MidSide *module)
{
    setSkinPath("res/MidSide.svg");
    initializeWidget(module, true);

    //////PARAMS//////

    //////INPUTS//////
    const float jackXLeft = 9;
    const float jackXRight = 57;

    createHCVTrimpot(jackXLeft + 3, 67 + 3, MidSide::WIDTH_PARAM);
    createInputPort(jackXRight, 67, MidSide::WIDTH_INPUT);
    

    const int inRow1 = 122;
    const int inRow2 = 242;

    createInputPort(jackXLeft, inRow1, MidSide::INL_INPUT);
    createInputPort(jackXRight, inRow1, MidSide::INR_INPUT);
    createInputPort(jackXLeft, inRow2, MidSide::INM_INPUT);
    createInputPort(jackXRight, inRow2, MidSide::INS_INPUT);

    const int outRow1 = 175;
    const int outRow2 = 293;
    createOutputPort(jackXLeft, outRow1, MidSide::OUTM_OUTPUT);
    createOutputPort(jackXRight, outRow1, MidSide::OUTS_OUTPUT);
    createOutputPort(jackXLeft, outRow2, MidSide::OUTL_OUTPUT);
    createOutputPort(jackXRight, outRow2, MidSide::OUTR_OUTPUT);
}

Model *modelMidSide = createModel<MidSide, MidSideWidget>("MidSide");
