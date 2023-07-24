#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorCommon.h"
#include "DSP/Phasors/HCVPhasorEffects.h"
#include "DSP/HCVTiming.h"

struct PhasorTimetable : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        DIV_2_OUTPUT,
        DIV_3_OUTPUT,
        DIV_4_OUTPUT,
        DIV_5_OUTPUT,
        DIV_8_OUTPUT,

        MULT_2_OUTPUT,
        MULT_3_OUTPUT,
        MULT_4_OUTPUT,
        MULT_5_OUTPUT,
        MULT_8_OUTPUT,
        
		NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS = NUM_OUTPUTS
	};

    HCVPhasorDivMult mult2[16];
    HCVPhasorDivMult mult3[16];
    HCVPhasorDivMult mult4[16];
    HCVPhasorDivMult mult5[16];
    HCVPhasorDivMult mult8[16];

    HCVPhasorDivMult div2[16];
    HCVPhasorDivMult div3[16];
    HCVPhasorDivMult div4[16];
    HCVPhasorDivMult div5[16];
    HCVPhasorDivMult div8[16];

    rack::dsp::SchmittTrigger resetTrigger[16];


	PhasorTimetable()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");
        configInput(RESET_INPUT, "Reset");

        configOutput(MULT_2_OUTPUT, "Phasor x2");
        configOutput(MULT_3_OUTPUT, "Phasor x3");
        configOutput(MULT_4_OUTPUT, "Phasor x4");
        configOutput(MULT_5_OUTPUT, "Phasor x5");
        configOutput(MULT_8_OUTPUT, "Phasor x8");

        configOutput(DIV_2_OUTPUT, "Phasor /2");
        configOutput(DIV_3_OUTPUT, "Phasor /3");
        configOutput(DIV_4_OUTPUT, "Phasor /4");
        configOutput(DIV_5_OUTPUT, "Phasor /5");
        configOutput(DIV_8_OUTPUT, "Phasor /8");

        for(int i = 0; i < 16; i++)
        {
            mult2[i].setMultiplier(2.0f);
            mult3[i].setMultiplier(3.0f);
            mult4[i].setMultiplier(4.0f);
            mult5[i].setMultiplier(5.0f);
            mult8[i].setMultiplier(8.0f);

            div2[i].setDivider(2.0f);
            div3[i].setDivider(3.0f);
            div4[i].setDivider(4.0f);
            div5[i].setDivider(5.0f);
            div8[i].setDivider(8.0f);
        }

	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorTimetable::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        if(resetTrigger[i].process(inputs[RESET_INPUT].getPolyVoltage(i)))
        {
            mult2[i].reset();
            mult3[i].reset();
            mult4[i].reset();
            mult5[i].reset();
            mult8[i].reset();

            div2[i].reset();
            div3[i].reset();
            div4[i].reset();
            div5[i].reset();
            div8[i].reset();
        }

        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        const float phasorMult2 = mult2[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorMult3 = mult3[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorMult4 = mult4[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorMult5 = mult5[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorMult8 = mult8[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;

        const float phasorDiv2  = div2[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorDiv3  = div3[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorDiv4  = div4[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorDiv5  = div5[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float phasorDiv8  = div8[i].basicSync(normalizedPhasor) * HCV_PHZ_UPSCALE;

        outputs[MULT_2_OUTPUT].setVoltage(phasorMult2, i);
        outputs[MULT_3_OUTPUT].setVoltage(phasorMult3, i);
        outputs[MULT_4_OUTPUT].setVoltage(phasorMult4, i);
        outputs[MULT_5_OUTPUT].setVoltage(phasorMult5, i);
        outputs[MULT_8_OUTPUT].setVoltage(phasorMult8, i);

        outputs[DIV_2_OUTPUT].setVoltage(phasorDiv2, i);
        outputs[DIV_3_OUTPUT].setVoltage(phasorDiv3, i);
        outputs[DIV_4_OUTPUT].setVoltage(phasorDiv4, i);
        outputs[DIV_5_OUTPUT].setVoltage(phasorDiv5, i);
        outputs[DIV_8_OUTPUT].setVoltage(phasorDiv8, i);
    }

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[i].setBrightness(outputs[i].getVoltage() * 0.1f);
    }
}

struct PhasorTimetableWidget : HCVModuleWidget { PhasorTimetableWidget(PhasorTimetable *module); };

PhasorTimetableWidget::PhasorTimetableWidget(PhasorTimetable *module)
{
    setSkinPath("res/PhasorTimetable.svg");
    initializeWidget(module);

    createInputPort(10, 62, PhasorTimetable::PHASOR_INPUT);
    createInputPort(56, 62, PhasorTimetable::RESET_INPUT);

    const int initialY = 130;
    const int initialLightY = 138;

    for(int i = 0; i < PhasorTimetable::NUM_OUTPUTS/2; i++)
    {
        const int yPos = i*42;
        createOutputPort(10, initialY + yPos, PhasorTimetable::DIV_2_OUTPUT + i);
        createOutputPort(56, initialY + yPos, PhasorTimetable::MULT_2_OUTPUT + i);

        createHCVRedLight(36, initialLightY + yPos, PhasorTimetable::DIV_2_OUTPUT + i);
        createHCVRedLight(48, initialLightY + yPos, PhasorTimetable::MULT_2_OUTPUT + i);
    }
}

Model *modelPhasorTimetable = createModel<PhasorTimetable, PhasorTimetableWidget>("PhasorTimetable");
