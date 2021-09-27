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
        configParam(MidSide::WIDTH_PARAM, -5.0, 5.0, 0.0, "");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void MidSide::process(const ProcessArgs &args)
{
    auto l = inputs[INL_INPUT].getVoltage();
    auto r = inputs[INR_INPUT].getVoltage();

    double width = normalizeParameter(inputs[WIDTH_INPUT].getVoltage() + params[WIDTH_PARAM].getValue());

    double mid = (l+r) * 0.5;
    double side = (r - l) * width;

    outputs[OUTM_OUTPUT].setVoltage(mid);
    outputs[OUTS_OUTPUT].setVoltage(side);

    if(inputs[INM_INPUT].isConnected()) mid = inputs[INM_INPUT].getVoltage();
    if(inputs[INS_INPUT].isConnected()) side = inputs[INS_INPUT].getVoltage();

    outputs[OUTL_OUTPUT].setVoltage(mid - side);
    outputs[OUTR_OUTPUT].setVoltage(mid + side);
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
