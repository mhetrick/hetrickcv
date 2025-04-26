#include "HetrickCV.hpp"

struct Normals : HCVModule
{
	enum ParamIds
	{

		NUM_PARAMS
	};
	enum InputIds
	{
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        IN7_INPUT,
        IN8_INPUT,

        NORMAL1_INPUT,
        NORMAL2_INPUT,
        NORMAL3_INPUT,
        NORMAL4_INPUT,
        NORMAL5_INPUT,
        NORMAL6_INPUT,
        NORMAL7_INPUT,
        NORMAL8_INPUT,

		NUM_INPUTS
	};
	enum OutputIds
	{
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,

		NUM_OUTPUTS
    };
    enum LightIds
	{

		NUM_LIGHTS
    };


	Normals()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 8; i++)
        {
            const auto channelString = std::to_string(i + 1);
            configInput(IN1_INPUT + i, "In " + channelString);
            configInput(NORMAL1_INPUT + i, "Normal " + channelString);
            configOutput(OUT1_OUTPUT + i, "Out " + channelString);
        }
	}

    void process(const ProcessArgs &args) override;

};


void Normals::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    for (int chan = 0; chan < numChannels; chan++)
    {
        for (int row = 0; row < 8; row++)
        {
            float input = inputs[IN1_INPUT + row].getPolyVoltage(chan);
            if (inputs[NORMAL1_INPUT + row].isConnected())
            {
                input = inputs[NORMAL1_INPUT + row].getPolyVoltage(chan);
            }
            outputs[OUT1_OUTPUT + row].setVoltage(input, chan);
        }
    }
}

struct NormalsWidget : HCVModuleWidget { NormalsWidget(Normals *module); };

NormalsWidget::NormalsWidget(Normals *module)
{
    setSkinPath("res/Normals.svg");
    initializeWidget(module);

    const int inXPos = 10;
    const int inLightX = 50;
    const int outXPos = 175;
    const int outLightX = 150;
    const int invParamX = 64;
    const int centerX = 83;

    for(int i = 0; i < 8; i++)
    {
        const int yPos = 50 + (40 * i);
        const int lightY = 59 + (40 * i);

        //////INPUTS//////
        createInputPort(inXPos, yPos, Normals::IN1_INPUT + i);
        createInputPort(centerX, yPos, Normals::NORMAL1_INPUT + i);

        //////OUTPUTS//////
        createOutputPort(outXPos, yPos, Normals::OUT1_OUTPUT + i);
    }
}

Model *modelNormals = createModel<Normals, NormalsWidget>("Normals");
