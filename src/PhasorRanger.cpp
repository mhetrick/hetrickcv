#include "HetrickCV.hpp"

#include "DSP/HCVPhasorEffects.h"

struct PhasorRanger : HCVModule
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
        BIPOLAR5V_OUTPUT,
        UNIPOLAR10V_OUTPUT,
        BIPOLAR10V_OUTPUT,
        UNIPOLAR1V_OUTPUT,
        BIPOLAR1V_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
	};


	PhasorRanger()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");
        configOutput(BIPOLAR5V_OUTPUT, "+/- 5V Phasor");
        configOutput(UNIPOLAR10V_OUTPUT, "0-10V Phasor");
        configOutput(BIPOLAR10V_OUTPUT, "+/- 10V Phasor");
        configOutput(UNIPOLAR1V_OUTPUT, "0-1V Phasor");
        configOutput(BIPOLAR1V_OUTPUT, "+/- 1V Phasor");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorRanger::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);
        const float bipolarPhasor = (normalizedPhasor * 2.0f) - 1.0f;

        outputs[BIPOLAR5V_OUTPUT].setVoltage(bipolarPhasor * 5.0f, i);
        outputs[UNIPOLAR10V_OUTPUT].setVoltage(normalizedPhasor * 10.0f, i);
        outputs[BIPOLAR10V_OUTPUT].setVoltage(bipolarPhasor * 10.0f, i);
        outputs[UNIPOLAR1V_OUTPUT].setVoltage(normalizedPhasor, i);
        outputs[BIPOLAR1V_OUTPUT].setVoltage(bipolarPhasor, i);
    }
    
}

struct PhasorRangerWidget : HCVModuleWidget { PhasorRangerWidget(PhasorRanger *module); };

PhasorRangerWidget::PhasorRangerWidget(PhasorRanger *module)
{
    setSkinPath("res/PhasorRanger.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorRanger::PHASOR_INPUT);

    for(int i = 0; i < PhasorRanger::NUM_OUTPUTS; i++)
    {
        const int yPos = i*42;
        createOutputPort(33, 115 + yPos, i);
    }
}

Model *modelPhasorRanger = createModel<PhasorRanger, PhasorRangerWidget>("PhasorRanger");
