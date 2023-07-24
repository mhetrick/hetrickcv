#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorCommon.h"

struct PhasorQuadrature : HCVModule
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

        SINE_0_OUTPUT,
        SINE_90_OUTPUT,
        SINE_180_OUTPUT,
        SINE_270_OUTPUT,
        SINE_INV_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS = NUM_OUTPUTS
	};


	PhasorQuadrature()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");

        configOutput(PHASOR_0_OUTPUT,   "Phasor");
        configOutput(PHASOR_90_OUTPUT,  "Phasor + 90 Degrees");
        configOutput(PHASOR_180_OUTPUT, "Phasor + 180 Degrees");
        configOutput(PHASOR_270_OUTPUT, "Phasor + 270 Degrees");
        configOutput(PHASOR_INV_OUTPUT, "Inverse Phasor");
        configOutput(SINE_0_OUTPUT,   "Sine");
        configOutput(SINE_90_OUTPUT,  "Phasor + 90 Degrees");
        configOutput(SINE_180_OUTPUT, "Phasor + 180 Degrees");
        configOutput(SINE_270_OUTPUT, "Phasor + 270 Degrees");
        configOutput(SINE_INV_OUTPUT, "Inverse Sine");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorQuadrature::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);

        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);
        const float phasor90  = gam::scl::wrap(normalizedPhasor + 0.250f);
        const float phasor180 = gam::scl::wrap(normalizedPhasor + 0.500f);
        const float phasor270 = gam::scl::wrap(normalizedPhasor + 0.750f);

        const float sine = sinf(normalizedPhasor * M_2PI) * 5.0f + 5.0f;
        const float sine90  = sinf(phasor90  * M_2PI) * 5.0f + 5.0f;
        const float sine180 = sinf(phasor180 * M_2PI) * 5.0f + 5.0f;
        const float sine270 = sinf(phasor270 * M_2PI) * 5.0f + 5.0f;

        outputs[PHASOR_0_OUTPUT].setVoltage(normalizedPhasor * HCV_PHZ_UPSCALE, i);
        outputs[PHASOR_90_OUTPUT].setVoltage(phasor90   * HCV_PHZ_UPSCALE, i);
        outputs[PHASOR_180_OUTPUT].setVoltage(phasor180 * HCV_PHZ_UPSCALE, i);
        outputs[PHASOR_270_OUTPUT].setVoltage(phasor270 * HCV_PHZ_UPSCALE, i);
        outputs[PHASOR_INV_OUTPUT].setVoltage((1.0f - normalizedPhasor) * HCV_PHZ_UPSCALE, i);

        outputs[SINE_0_OUTPUT].setVoltage(sine, i);
        outputs[SINE_90_OUTPUT].setVoltage(sine90, i);
        outputs[SINE_180_OUTPUT].setVoltage(sine180, i);
        outputs[SINE_270_OUTPUT].setVoltage(sine270, i);
        outputs[SINE_INV_OUTPUT].setVoltage(10.0f - sine, i);
    }

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        lights[i].setBrightness(outputs[i].getVoltage() * 0.1f);
    }
}

struct PhasorQuadratureWidget : HCVModuleWidget { PhasorQuadratureWidget(PhasorQuadrature *module); };

PhasorQuadratureWidget::PhasorQuadratureWidget(PhasorQuadrature *module)
{
    setSkinPath("res/PhasorQuadrature.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorQuadrature::PHASOR_INPUT);

    const int initialY = 130;
    const int initialLightY = 138;

    for(int i = 0; i < PhasorQuadrature::NUM_OUTPUTS/2; i++)
    {
        const int yPos = i*42;
        createOutputPort(10, initialY + yPos, PhasorQuadrature::PHASOR_0_OUTPUT + i);
        createOutputPort(56, initialY + yPos, PhasorQuadrature::SINE_0_OUTPUT + i);

        createHCVRedLight(36, initialLightY + yPos, PhasorQuadrature::PHASOR_0_OUTPUT + i);
        createHCVRedLight(48, initialLightY + yPos, PhasorQuadrature::SINE_0_OUTPUT + i);
    }
}

Model *modelPhasorQuadrature = createModel<PhasorQuadrature, PhasorQuadratureWidget>("PhasorQuadrature");
