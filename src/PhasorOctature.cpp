#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorCommon.h"

struct PhasorOctature : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_0_OUTPUT,
        PHASOR_90_OUTPUT,
        PHASOR_180_OUTPUT,
        PHASOR_270_OUTPUT,
        PHASOR_INV_OUTPUT,

        PHASOR_45_OUTPUT,
        PHASOR_135_OUTPUT,
        PHASOR_225_OUTPUT,
        PHASOR_315_OUTPUT,
        PHASOR_180_INV_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS = NUM_OUTPUTS
	};


	PhasorOctature()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");

        configOutput(PHASOR_0_OUTPUT,   "Phasor");
        configOutput(PHASOR_45_OUTPUT,  "Phasor + 45 Degrees");
        configOutput(PHASOR_90_OUTPUT,  "Phasor + 90 Degrees");
        configOutput(PHASOR_135_OUTPUT, "Phasor + 135 Degrees");
        configOutput(PHASOR_180_OUTPUT, "Phasor + 180 Degrees");
        configOutput(PHASOR_225_OUTPUT, "Phasor + 225 Degrees");
        configOutput(PHASOR_270_OUTPUT, "Phasor + 270 Degrees");
        configOutput(PHASOR_315_OUTPUT, "Phasor + 315 Degrees");

        configOutput(PHASOR_INV_OUTPUT, "Inverse Phasor");
        configOutput(PHASOR_180_INV_OUTPUT, "Inverse Phasor + 180 Degrees");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorOctature::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        const float phasor45  = gam::scl::wrap(normalizedPhasor + 0.125f) * HCV_PHZ_UPSCALE;
        const float phasor90  = gam::scl::wrap(normalizedPhasor + 0.250f) * HCV_PHZ_UPSCALE;
        const float phasor135 = gam::scl::wrap(normalizedPhasor + 0.375f) * HCV_PHZ_UPSCALE;
        const float phasor180 = gam::scl::wrap(normalizedPhasor + 0.500f) * HCV_PHZ_UPSCALE;
        const float phasor225 = gam::scl::wrap(normalizedPhasor + 0.625f) * HCV_PHZ_UPSCALE;
        const float phasor270 = gam::scl::wrap(normalizedPhasor + 0.750f) * HCV_PHZ_UPSCALE;
        const float phasor315 = gam::scl::wrap(normalizedPhasor + 0.825f) * HCV_PHZ_UPSCALE;

        outputs[PHASOR_0_OUTPUT].setVoltage(normalizedPhasor * HCV_PHZ_UPSCALE, i);
        outputs[PHASOR_90_OUTPUT].setVoltage(phasor90, i);
        outputs[PHASOR_180_OUTPUT].setVoltage(phasor180, i);
        outputs[PHASOR_270_OUTPUT].setVoltage(phasor270, i);
        outputs[PHASOR_INV_OUTPUT].setVoltage((1.0f - normalizedPhasor) * HCV_PHZ_UPSCALE, i);

        outputs[PHASOR_45_OUTPUT].setVoltage(phasor45, i);
        outputs[PHASOR_135_OUTPUT].setVoltage(phasor135, i);
        outputs[PHASOR_225_OUTPUT].setVoltage(phasor225, i);
        outputs[PHASOR_315_OUTPUT].setVoltage(phasor315, i);
        outputs[PHASOR_180_INV_OUTPUT].setVoltage(10.0f - phasor180, i);
    }

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[i].setBrightness(outputs[i].getVoltage() * 0.1f);
    }
}

struct PhasorOctatureWidget : HCVModuleWidget { PhasorOctatureWidget(PhasorOctature *module); };

PhasorOctatureWidget::PhasorOctatureWidget(PhasorOctature *module)
{
    setSkinPath("res/PhasorOctature.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorOctature::PHASOR_INPUT);

    const int initialY = 130;
    const int initialLightY = 138;

    for(int i = 0; i < PhasorOctature::NUM_OUTPUTS/2; i++)
    {
        const int yPos = i*42;
        createOutputPort(10, initialY + yPos, PhasorOctature::PHASOR_0_OUTPUT + i);
        createOutputPort(56, initialY + yPos, PhasorOctature::PHASOR_45_OUTPUT + i);

        createHCVRedLight(36, initialLightY + yPos, PhasorOctature::PHASOR_0_OUTPUT + i);
        createHCVRedLight(48, initialLightY + yPos, PhasorOctature::PHASOR_45_OUTPUT + i);
    }
}

Model *modelPhasorOctature = createModel<PhasorOctature, PhasorOctatureWidget>("PhasorOctature");
