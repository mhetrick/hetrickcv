#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorCommon.h"

struct PhasorToWaveforms : HCVModule
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
        SINE_OUTPUT,
        TRIANGLE_OUTPUT,
        SAW_OUTPUT,
        RAMP_OUTPUT,
        SQUARE_OUTPUT,

        SINE_BIPOLAR_OUTPUT,
        TRIANGLE_BIPOLAR_OUTPUT,
        SAW_BIPOLAR_OUTPUT,
        RAMP_BIPOLAR_OUTPUT,
        SQUARE_BIPOLAR_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        SINE_LIGHT,
        TRIANGLE_LIGHT,
        SAW_LIGHT,
        RAMP_LIGHT,
        SQUARE_LIGHT,

        ENUMS(SINE_BIPOLAR_LIGHT, 2),
        ENUMS(TRIANGLE_BIPOLAR_LIGHT, 2),
        ENUMS(SAW_BIPOLAR_LIGHT, 2),
        ENUMS(RAMP_BIPOLAR_LIGHT, 2),
        ENUMS(SQUARE_BIPOLAR_LIGHT, 2),
        NUM_LIGHTS
	};


	PhasorToWaveforms()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");

        configOutput(SINE_OUTPUT, "Unipolar Sine");
        configOutput(TRIANGLE_OUTPUT, "Unipolar Triangle");
        configOutput(SAW_OUTPUT, "Unipolar Saw");
        configOutput(RAMP_OUTPUT, "Unipolar Ramp");
        configOutput(SQUARE_OUTPUT, "Unipolar Square");

        configOutput(SINE_BIPOLAR_OUTPUT, "Bipolar Sine");
        configOutput(TRIANGLE_BIPOLAR_OUTPUT, "Bipolar Triangle");
        configOutput(SAW_BIPOLAR_OUTPUT, "Bipolar Saw");
        configOutput(RAMP_BIPOLAR_OUTPUT, "Bipolar Ramp");
        configOutput(SQUARE_BIPOLAR_OUTPUT, "Bipolar Square");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorToWaveforms::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        const float sine = sinf(normalizedPhasor * M_2PI) * 5.0f;
        const float saw = normalizedPhasor * HCV_PHZ_UPSCALE;
        const float ramp = (1.0f - normalizedPhasor) * HCV_PHZ_UPSCALE;
        const float triangle = (gam::scl::fold(normalizedPhasor * 2.0f) * HCV_PHZ_UPSCALE);
        const float square = normalizedPhasor < 0.5f ? HCV_PHZ_GATESCALE : 0.0f;

        outputs[SINE_OUTPUT].setVoltage(sine + 5.0f, i);
        outputs[TRIANGLE_OUTPUT].setVoltage(triangle, i);
        outputs[SAW_OUTPUT].setVoltage(saw, i);
        outputs[RAMP_OUTPUT].setVoltage(ramp, i);
        outputs[SQUARE_OUTPUT].setVoltage(square, i);

        outputs[SINE_BIPOLAR_OUTPUT].setVoltage(sine, i);
        outputs[TRIANGLE_BIPOLAR_OUTPUT].setVoltage(triangle - 5.0f, i);
        outputs[SAW_BIPOLAR_OUTPUT].setVoltage(saw - 5.0f, i);
        outputs[RAMP_BIPOLAR_OUTPUT].setVoltage(ramp - 5.0f, i);
        outputs[SQUARE_BIPOLAR_OUTPUT].setVoltage(square - 5.0f, i);
    }

    for (int i = 0; i < 5; i++)
    {
        lights[SINE_LIGHT + i].setBrightness(outputs[i].getVoltage() * 0.1f);
        setBipolarLightBrightness(SINE_BIPOLAR_LIGHT + (i*2), outputs[SINE_BIPOLAR_OUTPUT + i].getVoltage() * 0.2f);
    }
}

struct PhasorToWaveformsWidget : HCVModuleWidget { PhasorToWaveformsWidget(PhasorToWaveforms *module); };

PhasorToWaveformsWidget::PhasorToWaveformsWidget(PhasorToWaveforms *module)
{
    setSkinPath("res/PhasorToWaveforms.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorToWaveforms::PHASOR_INPUT);

    const int initialY = 130;
    const int initialLightY = 138;

    for(int i = 0; i < 5; i++)
    {
        const int yPos = i*42;
        createOutputPort(10, initialY + yPos, PhasorToWaveforms::SINE_OUTPUT + i);
        createOutputPort(56, initialY + yPos, PhasorToWaveforms::SINE_BIPOLAR_OUTPUT + i);

        createHCVRedLight(36, initialLightY + yPos, PhasorToWaveforms::SINE_LIGHT + i);
        createHCVBipolarLight(48, initialLightY + yPos, PhasorToWaveforms::SINE_BIPOLAR_LIGHT + (i*2));
    }
}

Model *modelPhasorToWaveforms = createModel<PhasorToWaveforms, PhasorToWaveformsWidget>("PhasorToWaveforms");
