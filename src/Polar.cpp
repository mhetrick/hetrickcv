#include "HetrickCV.hpp"

        

struct XYToPolar : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        INX_INPUT,
        INY_INPUT,
        INR_INPUT,
        INTHETA_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUTR_OUTPUT,
        OUTTHETA_OUTPUT,
        OUTX_OUTPUT,
        OUTY_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
	};

	XYToPolar()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override;

    simd::float_4   insX[4] = {0.0f, 0.0f, 0.0f, 0.0f}, insY[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    insR[4] = {0.0f, 0.0f, 0.0f, 0.0f}, insTheta[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outsX[4] = {0.0f, 0.0f, 0.0f, 0.0f}, outsY[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outsR[4] = {0.0f, 0.0f, 0.0f, 0.0f}, outsTheta[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void XYToPolar::process(const ProcessArgs &args)
{
    int channels = getMaxInputPolyphony();
    outputs[OUTX_OUTPUT].setChannels(channels);
    outputs[OUTY_OUTPUT].setChannels(channels);
    outputs[OUTR_OUTPUT].setChannels(channels);
    outputs[OUTTHETA_OUTPUT].setChannels(channels);

    bool rConnected = inputs[INR_INPUT].isConnected();
    bool thetaConnected = inputs[INTHETA_INPUT].isConnected();

	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c / 4;
		insX[vectorIndex] = simd::float_4::load(inputs[INX_INPUT].getVoltages(c));
        insY[vectorIndex] = simd::float_4::load(inputs[INY_INPUT].getVoltages(c));

        outsR[vectorIndex] = sqrt(insX[vectorIndex] * insX[vectorIndex] + insY[vectorIndex] * insY[vectorIndex]);
        outsTheta[vectorIndex] = atan2(insY[vectorIndex], insX[vectorIndex]);

        for (int i = 0; i < 4; i++)
        {
            if(!std::isnormal(outsTheta[vectorIndex][i])) outsTheta[vectorIndex][i] = 0.0f;
        }

        insR[vectorIndex] = rConnected ? simd::float_4::load(inputs[INR_INPUT].getVoltages(c)) : outsR[vectorIndex];
        insTheta[vectorIndex] = thetaConnected ? simd::float_4::load(inputs[INTHETA_INPUT].getVoltages(c)) : outsTheta[vectorIndex];

        outsX[vectorIndex] = insR[vectorIndex] * cos(insTheta[vectorIndex]);
        outsY[vectorIndex] = insR[vectorIndex] * sin(insTheta[vectorIndex]);

        outsR[vectorIndex].store(outputs[OUTR_OUTPUT].getVoltages(c));
        outsTheta[vectorIndex].store(outputs[OUTTHETA_OUTPUT].getVoltages(c));
        outsX[vectorIndex].store(outputs[OUTX_OUTPUT].getVoltages(c));
        outsY[vectorIndex].store(outputs[OUTY_OUTPUT].getVoltages(c));
	}
}

struct XYToPolarWidget : HCVModuleWidget { XYToPolarWidget(XYToPolar *module); };

XYToPolarWidget::XYToPolarWidget(XYToPolar *module)
{
    setSkinPath("res/XYToPolar.svg");
    initializeWidget(module, true);

    //////PARAMS//////

    //////INPUTS//////
    const float jackXLeft = 9;
    const float jackXRight = 57;

    const int inRow1 = 62;
    const int inRow2 = 199;

    createInputPort(jackXLeft, inRow1, XYToPolar::INX_INPUT);
    createInputPort(jackXRight, inRow1, XYToPolar::INY_INPUT);
    createInputPort(jackXLeft, inRow2, XYToPolar::INR_INPUT);
    createInputPort(jackXRight, inRow2, XYToPolar::INTHETA_INPUT);

    const int outRow1 = 116;
    const int outRow2 = 251;
    createOutputPort(jackXLeft, outRow1, XYToPolar::OUTR_OUTPUT);
    createOutputPort(jackXRight, outRow1, XYToPolar::OUTTHETA_OUTPUT);
    createOutputPort(jackXLeft, outRow2, XYToPolar::OUTX_OUTPUT);
    createOutputPort(jackXRight, outRow2, XYToPolar::OUTY_OUTPUT);
}

Model *modelXYToPolar = createModel<XYToPolar, XYToPolarWidget>("XYToPolar");
