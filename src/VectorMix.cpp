#include "HetrickCV.hpp"
#include "dsp/digital.hpp"
                     

struct VectorMix : HCVModule
{
	enum ParamIds
	{
        GAINA_PARAM,
        GAINB_PARAM,
        GAINC_PARAM,
        GAIND_PARAM,

        X_PARAM,
        Y_PARAM,

        XCV_PARAM,
        YCV_PARAM,

        OFFSET_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        INA_INPUT,
        INB_INPUT,
        INC_INPUT,
        IND_INPUT,

        INX_INPUT,
        INY_INPUT,

        ALL_INPUT,

		NUM_INPUTS
	};
	enum OutputIds
	{
        OUTMAIN_OUTPUT,
        OUTA_OUTPUT,
        OUTB_OUTPUT,
        OUTC_OUTPUT,
        OUTD_OUTPUT,

		NUM_OUTPUTS
	};

	enum LightIds
    {
        POSA_LIGHT,
        POSB_LIGHT,
        POSC_LIGHT,
        POSD_LIGHT,

        OFFSET_LIGHT,

        OUTA_POS_LIGHT, OUTA_NEG_LIGHT,
		OUTB_POS_LIGHT, OUTB_NEG_LIGHT,
		OUTC_POS_LIGHT, OUTC_NEG_LIGHT,
		OUTD_POS_LIGHT, OUTD_NEG_LIGHT,
        
        NUM_LIGHTS
	};

	VectorMix()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(VectorMix::GAINA_PARAM, 0.0, 1.0, 1.0, "A Gain");
        configParam(VectorMix::GAINB_PARAM, 0.0, 1.0, 1.0, "B Gain");
        configParam(VectorMix::GAINC_PARAM, 0.0, 1.0, 1.0, "C Gain");
        configParam(VectorMix::GAIND_PARAM, 0.0, 1.0, 1.0, "D Gain");

        configParam(VectorMix::X_PARAM, 0.0, 1.0, 0.5, "X Position");
		configParam(VectorMix::XCV_PARAM, -1.0, 1.0, 1.0, "X Position CV Depth");

        configParam(VectorMix::Y_PARAM, 0.0, 1.0, 0.5, "Y Position");
		configParam(VectorMix::YCV_PARAM, -1.0, 1.0, 1.0, "Y Position CV Depth");

        configSwitch(VectorMix::OFFSET_PARAM, 0.0, 1.0, 0.0, "Enable Input Offset", {"0V", "5V"});

        configInput(INA_INPUT, "A");
        configInput(INB_INPUT, "B");
        configInput(INC_INPUT, "C");
        configInput(IND_INPUT, "D");

        configInput(INX_INPUT, "X");
        configInput(INY_INPUT, "Y");

        configInput(ALL_INPUT, "All unplugged channels");

        configOutput(OUTMAIN_OUTPUT, "Mix");

        configOutput(OUTA_OUTPUT, "A");
        configOutput(OUTB_OUTPUT, "B");
        configOutput(OUTC_OUTPUT, "C");
        configOutput(OUTD_OUTPUT, "D");
	}

    simd::float_4   insA[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    insB[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    insC[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    insD[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    insAll[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    insX[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    insY[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outMultsA[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outMultsB[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outMultsC[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outMultsD[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    abMix[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    cdMix[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outA[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outB[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outC[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    outD[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outMix[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void VectorMix::process(const ProcessArgs &args)
{

    int channels = getMaxInputPolyphony();

    //TODO: Interpolation
    const float gainA = params[GAINA_PARAM].getValue();
    const float gainB = params[GAINB_PARAM].getValue();
    const float gainC = params[GAINC_PARAM].getValue();
    const float gainD = params[GAIND_PARAM].getValue();

    const float offsetValue = params[OFFSET_PARAM].getValue() > 0.0f ? 5.0f : 0.0f;
    lights[OFFSET_LIGHT].setBrightness(offsetValue);

    const float xKnob = params[X_PARAM].getValue();
    const float xScale = params[XCV_PARAM].getValue() * 0.2;

    const float yKnob = params[Y_PARAM].getValue();
    const float yScale = params[YCV_PARAM].getValue() * 0.2;

    outputs[OUTMAIN_OUTPUT].setChannels(channels);
    outputs[OUTA_OUTPUT].setChannels(channels);
    outputs[OUTB_OUTPUT].setChannels(channels);
    outputs[OUTC_OUTPUT].setChannels(channels);
    outputs[OUTD_OUTPUT].setChannels(channels);


	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c/4;

        insX[vectorIndex] = simd::float_4::load(inputs[INX_INPUT].getVoltages(c));
		insX[vectorIndex] = (insX[vectorIndex] * xScale) + xKnob;
        insX[vectorIndex] = clamp(insX[vectorIndex], 0.0f, 1.0f);

        insY[vectorIndex] = simd::float_4::load(inputs[INY_INPUT].getVoltages(c));
		insY[vectorIndex] = (insY[vectorIndex] * yScale) + yKnob;
        insY[vectorIndex] = clamp(insY[vectorIndex], 0.0f, 1.0f);

        if(inputs[ALL_INPUT].isConnected())
        {
            insAll[vectorIndex] = simd::float_4::load(inputs[ALL_INPUT].getVoltages(c));
        }
        else
        {
            insAll[vectorIndex] = {offsetValue, offsetValue, offsetValue, offsetValue};
        }

		insA[vectorIndex] = inputs[INA_INPUT].isConnected() ? simd::float_4::load(inputs[INA_INPUT].getVoltages(c)) : insAll[vectorIndex];
        insB[vectorIndex] = inputs[INB_INPUT].isConnected() ? simd::float_4::load(inputs[INB_INPUT].getVoltages(c)) : insAll[vectorIndex];
        insC[vectorIndex] = inputs[INC_INPUT].isConnected() ? simd::float_4::load(inputs[INC_INPUT].getVoltages(c)) : insAll[vectorIndex];
        insD[vectorIndex] = inputs[IND_INPUT].isConnected() ? simd::float_4::load(inputs[IND_INPUT].getVoltages(c)) : insAll[vectorIndex];

        insA[vectorIndex] *= gainA;
        insB[vectorIndex] *= gainB;
        insC[vectorIndex] *= gainC;
        insD[vectorIndex] *= gainD;

        outMultsA[vectorIndex] = (1.0f - insX[vectorIndex]) * (1.0f - insY[vectorIndex]);
        outMultsB[vectorIndex] = insX[vectorIndex] * (1.0f - insY[vectorIndex]);
        outMultsC[vectorIndex] = (1.0f - insX[vectorIndex]) * insY[vectorIndex];
        outMultsD[vectorIndex] = insX[vectorIndex] * insY[vectorIndex];

        outA[vectorIndex] = insA[vectorIndex] * outMultsA[vectorIndex];
        outB[vectorIndex] = insB[vectorIndex] * outMultsB[vectorIndex];
        outC[vectorIndex] = insC[vectorIndex] * outMultsC[vectorIndex];
        outD[vectorIndex] = insD[vectorIndex] * outMultsD[vectorIndex];

        outMix[vectorIndex] = outA[vectorIndex] + outB[vectorIndex] + outC[vectorIndex] + outD[vectorIndex];

        outMix[vectorIndex].store(outputs[OUTMAIN_OUTPUT].getVoltages(c));
        outA[vectorIndex].store(outputs[OUTA_OUTPUT].getVoltages(c));
        outB[vectorIndex].store(outputs[OUTB_OUTPUT].getVoltages(c));
        outC[vectorIndex].store(outputs[OUTC_OUTPUT].getVoltages(c));
        outD[vectorIndex].store(outputs[OUTD_OUTPUT].getVoltages(c));
	}

    lights[POSA_LIGHT].setBrightness(outMultsA[0][0]);
    lights[POSB_LIGHT].setBrightness(outMultsB[0][0]);
    lights[POSC_LIGHT].setBrightness(outMultsC[0][0]);
    lights[POSD_LIGHT].setBrightness(outMultsD[0][0]);

	lights[OUTA_POS_LIGHT].setBrightness(fmax(0.0f,  outA[0][0]));
    lights[OUTA_NEG_LIGHT].setBrightness(fmax(0.0f, -outA[0][0]));

    lights[OUTB_POS_LIGHT].setBrightness(fmax(0.0f,  outB[0][0]));
    lights[OUTB_NEG_LIGHT].setBrightness(fmax(0.0f, -outB[0][0]));

    lights[OUTC_POS_LIGHT].setBrightness(fmax(0.0f,  outC[0][0]));
    lights[OUTC_NEG_LIGHT].setBrightness(fmax(0.0f, -outC[0][0]));

    lights[OUTD_POS_LIGHT].setBrightness(fmax(0.0f,  outD[0][0]));
    lights[OUTD_NEG_LIGHT].setBrightness(fmax(0.0f, -outD[0][0]));

}

struct VectorMixWidget : HCVModuleWidget { VectorMixWidget(VectorMix *module); };

VectorMixWidget::VectorMixWidget(VectorMix *module)
{
	setSkinPath("res/VectorMix.svg");
    initializeWidget(module);

    //////PARAMS//////

    //////INPUTS//////
    int acX = 145;
    int bdX = 200;
    int abY = 65;
    int cdY = 150;

    createInputPort(acX, abY, VectorMix::INA_INPUT);
    createInputPort(bdX, abY, VectorMix::INB_INPUT);
    createInputPort(acX, cdY, VectorMix::INC_INPUT);
    createInputPort(bdX, cdY, VectorMix::IND_INPUT);

    int knobOffsetX = 2;
    int knobOffsetY = 30;
    createHCVTrimpot(acX + knobOffsetX, abY + knobOffsetY, VectorMix::GAINA_PARAM);
    createHCVTrimpot(bdX + knobOffsetX, abY + knobOffsetY, VectorMix::GAINB_PARAM);
    createHCVTrimpot(acX + knobOffsetX, cdY + knobOffsetY, VectorMix::GAINC_PARAM);
    createHCVTrimpot(bdX + knobOffsetX, cdY + knobOffsetY, VectorMix::GAIND_PARAM);

    createParamComboVertical(12, abY + 2, VectorMix::X_PARAM, VectorMix::XCV_PARAM, VectorMix::INX_INPUT);
    createParamComboVertical(67, abY + 2, VectorMix::Y_PARAM, VectorMix::YCV_PARAM, VectorMix::INY_INPUT);


    abY = 250;
    cdY = 300;

    createHCVSwitchHoriz(12, cdY + 8, VectorMix::OFFSET_PARAM);
    createHCVRedLight(19, cdY - 2, VectorMix::OFFSET_LIGHT);

    createInputPort(53, cdY, VectorMix::ALL_INPUT);
    createOutputPort(100, cdY, VectorMix::OUTMAIN_OUTPUT);

    createOutputPort(acX, abY, VectorMix::OUTA_OUTPUT);
    createOutputPort(bdX, abY, VectorMix::OUTB_OUTPUT);
    createOutputPort(acX, cdY, VectorMix::OUTC_OUTPUT);
    createOutputPort(bdX, cdY, VectorMix::OUTD_OUTPUT);

    createHCVRedLight(33, 220, VectorMix::POSA_LIGHT);
    createHCVRedLight(73, 220, VectorMix::POSB_LIGHT);
    createHCVRedLight(33, 260, VectorMix::POSC_LIGHT);
    createHCVRedLight(73, 260, VectorMix::POSD_LIGHT);

    createHCVBipolarLight(acX + 27, abY + 8, VectorMix::OUTA_POS_LIGHT);
    createHCVBipolarLight(bdX - 10, abY + 8, VectorMix::OUTB_POS_LIGHT);
    createHCVBipolarLight(acX + 27, cdY + 8, VectorMix::OUTC_POS_LIGHT);
    createHCVBipolarLight(bdX - 10, cdY + 8, VectorMix::OUTD_POS_LIGHT);
}

Model *modelVectorMix = createModel<VectorMix, VectorMixWidget>("VectorMix");

