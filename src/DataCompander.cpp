#include "HetrickCV.hpp"

        

struct DataCompander : HCVModule
{
	enum ParamIds
	{
        COMP_MODE_PARAM,
        EXP_MODE_PARAM,
        COMP_RANGE,
        EXP_RANGE,
		NUM_PARAMS
	};
	enum InputIds
	{
        EXP_INL_INPUT,
        EXP_INR_INPUT,
        COMP_INL_INPUT,
        COMP_INR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        EXP_OUTL_OUTPUT,
        EXP_OUTR_OUTPUT,
        COMP_OUTL_OUTPUT,
        COMP_OUTR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
	};

	DataCompander()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(COMP_MODE_PARAM, 0.0, 2.0, 0.0, "Compression Mode", {"None", "A-Law", "mu-Law"});
        configSwitch(EXP_MODE_PARAM, 0.0, 2.0, 0.0, "Expansion Mode", {"None", "A-Law", "mu-Law"});
        configSwitch(COMP_RANGE, 0.0, 1.0, 0.0, "Compression Voltage Range", {"5V", "10V"});
        configSwitch(EXP_RANGE, 0.0, 1.0, 0.0, "Expansion Voltage Range", {"5V", "10V"});
	}

	void process(const ProcessArgs &args) override;

    simd::float_4   compInsL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, compInsR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    expInsL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, expInsR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    compOutsL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, compOutsR[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    expOutsL[4] = {0.0f, 0.0f, 0.0f, 0.0f}, expOutsR[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    const float mu = 255.0f;
    const float logMu1 = log(mu + 1.0f);
    const float recipMu = 1.0f/mu;

    const float A = 87.6f;
    const float recipA = 1.0f/A;
    const float logA = log(A);
    const float aExpansionCompare = (1.0f/(1.0f + logA));

    simd::float_4 aLawCompression(simd::float_4 _input)
    {
        simd::float_4 absx = fabs(_input);

        for (int i = 0; i < 4; i++)
        {
            if (absx[i] < recipA)
            {
                _input[i] = fabs((A * absx[i]) / (1 + logA)) * sgn(_input[i]);
            }
            else
            {
                _input[i] = fabs((1 + log(A * absx[i])) / (1 + logA)) * sgn(_input[i]);
            }
        }
        
        return _input;
    }

    simd::float_4 muLawCompression(simd::float_4 _input)
    {
        return fabs((log(1.0f + mu * fabs(_input))) / logMu1) * sgn(_input);
    }

    simd::float_4 aLawExpansion(simd::float_4 _input)
    {
        simd::float_4 absy = fabs(_input);

        for (int i = 0; i < 4; i++)
        {
            if (absy[i] < aExpansionCompare)
            {
                _input[i] = fabs((absy[i] * (1.0f + logA)) / A) * sgn(_input[i]);
            }
            else
            {
                _input[i] = fabs(exp(absy[i] * (1.0f + logA) - 1.0f) / A) * sgn(_input[i]);
            }
        }

        return _input;
    }

    simd::float_4 muLawExpansion(simd::float_4 _input)
    {
        return fabs(recipMu * (pow(1.0 + mu, fabs(_input)) - 1.0f)) * sgn(_input);
    }

    simd::float_4 compress(simd::float_4 _input)
    {
        switch (compMode)
        {
        case 1:
            return aLawCompression(_input);
            break;
        
        case 2:
            return muLawCompression(_input);
            break;

        default:
            return _input;
            break;
        }
    }

    simd::float_4 expand(simd::float_4 _input)
    {
        switch (expMode)
        {
        case 1:
            return aLawExpansion(_input);
            break;
        
        case 2:
            return muLawExpansion(_input);
            break;

        default:
            return _input;
            break;
        }
    }

    float compUpscale = 5.0f;
	float compDownscale = 0.2f;

    float expUpscale = 5.0f;
	float expDownscale = 0.2f;

    int compMode = 0;
    int expMode = 0;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void DataCompander::process(const ProcessArgs &args)
{
    if (params[COMP_RANGE].getValue() == 0.0f)
	{
		compUpscale = 5.0f;
		compDownscale = 0.2f;
	}
	else
	{
		compUpscale = 10.0f;
		compDownscale = 0.1f;
	}

    if (params[EXP_RANGE].getValue() == 0.0f)
	{
		expUpscale = 5.0f;
		expDownscale = 0.2f;
	}
	else
	{
		expUpscale = 10.0f;
		expDownscale = 0.1f;
	}

    int channels = getMaxInputPolyphony();
    outputs[COMP_OUTL_OUTPUT].setChannels(channels);
    outputs[COMP_OUTR_OUTPUT].setChannels(channels);
    outputs[EXP_OUTL_OUTPUT].setChannels(channels);
    outputs[EXP_OUTR_OUTPUT].setChannels(channels);

    bool expLConnected = inputs[EXP_INL_INPUT].isConnected();
    bool expRConnected = inputs[EXP_INR_INPUT].isConnected();

    expMode = params[EXP_MODE_PARAM].getValue();
    compMode = params[COMP_MODE_PARAM].getValue();

	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c / 4;

        ////compression
		compInsL[vectorIndex] = simd::float_4::load(inputs[COMP_INL_INPUT].getVoltages(c));
        compInsR[vectorIndex] = simd::float_4::load(inputs[COMP_INR_INPUT].getVoltages(c));

        compInsL[vectorIndex] = clamp(compInsL[vectorIndex] * compDownscale, -1.0f, 1.0f);
        compInsR[vectorIndex] = clamp(compInsR[vectorIndex] * compDownscale, -1.0f, 1.0f);

        compOutsL[vectorIndex] = compress(compInsL[vectorIndex]) * compUpscale;
        compOutsR[vectorIndex] = compress(compInsR[vectorIndex]) * compUpscale;

        ////expansion

        expInsL[vectorIndex] = expLConnected ? simd::float_4::load(inputs[EXP_INL_INPUT].getVoltages(c)) : compOutsL[vectorIndex];
        expInsR[vectorIndex] = expRConnected ? simd::float_4::load(inputs[EXP_INR_INPUT].getVoltages(c)) : compOutsR[vectorIndex];

        expInsL[vectorIndex] = clamp(expInsL[vectorIndex] * expDownscale, -1.0f, 1.0f);
        expInsR[vectorIndex] = clamp(expInsR[vectorIndex] * expDownscale, -1.0f, 1.0f);

        expOutsL[vectorIndex] = expand(expInsL[vectorIndex]) * expUpscale;
        expOutsR[vectorIndex] = expand(expInsR[vectorIndex]) * expUpscale;

        compOutsL[vectorIndex].store(outputs[COMP_OUTL_OUTPUT].getVoltages(c));
        compOutsR[vectorIndex].store(outputs[COMP_OUTR_OUTPUT].getVoltages(c));
        expOutsL[vectorIndex].store(outputs[EXP_OUTL_OUTPUT].getVoltages(c));
        expOutsR[vectorIndex].store(outputs[EXP_OUTR_OUTPUT].getVoltages(c));
	}
}

struct DataCompanderWidget : HCVModuleWidget { DataCompanderWidget(DataCompander *module); };

DataCompanderWidget::DataCompanderWidget(DataCompander *module)
{
    setSkinPath("res/DataCompander.svg");
    initializeWidget(module, true);

    //////PARAMS//////

    //////INPUTS//////
    const float jackXLeft = 9;
    const float jackXRight = 57;

    const int inRow1 = 62;
    const int inRow2 = 199;

    createInputPort(jackXLeft, inRow1, DataCompander::COMP_INL_INPUT);
    createInputPort(jackXRight, inRow1, DataCompander::COMP_INR_INPUT);
    createInputPort(jackXLeft, inRow2, DataCompander::EXP_INL_INPUT);
    createInputPort(jackXRight, inRow2, DataCompander::EXP_INR_INPUT);

    int switchX1 = 14;
    int switchX2 = 62;

    addParam(createParam<CKSSThree>(Vec(switchX1, 105.0f), module, DataCompander::COMP_MODE_PARAM));
    addParam(createParam<CKSSThree>(Vec(switchX1, 240.0f), module, DataCompander::EXP_MODE_PARAM));

    createHCVSwitchVert(switchX2, 108.0f, DataCompander::COMP_RANGE);
    createHCVSwitchVert(switchX2, 243.0f, DataCompander::EXP_RANGE);

    const int outRow1 = 146;
    const int outRow2 = 283;
    createOutputPort(jackXLeft, outRow1, DataCompander::COMP_OUTL_OUTPUT);
    createOutputPort(jackXRight, outRow1, DataCompander::COMP_OUTR_OUTPUT);
    createOutputPort(jackXLeft, outRow2, DataCompander::EXP_OUTL_OUTPUT);
    createOutputPort(jackXRight, outRow2, DataCompander::EXP_OUTR_OUTPUT);
}

Model *modelDataCompander = createModel<DataCompander, DataCompanderWidget>("DataCompander");
