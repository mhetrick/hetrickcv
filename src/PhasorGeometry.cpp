#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorGeometry : HCVModule
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
        REVERSE_OUTPUT,
        PINGPONG_OUTPUT,
        PONGPING_OUTPUT,
        MULT2_OUTPUT,
        MULT4_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
	};


	PhasorGeometry()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");
        configOutput(REVERSE_OUTPUT, "Reverse Phasor");
        configOutput(PINGPONG_OUTPUT, "Ping-Pong Phasor");
        configOutput(PONGPING_OUTPUT, "Pong-Ping Phasor");
        configOutput(MULT2_OUTPUT, "x2 Phasor");
        configOutput(MULT4_OUTPUT, "x4 Phasor");
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorGeometry::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);
        const float reversed = 1.0f - normalizedPhasor;
        const float pingPong = gam::scl::fold(normalizedPhasor * 2.0f);
        const float pongPing = 1.0f - pingPong;
        const float x2 = gam::scl::wrap(normalizedPhasor * 2.0f);
        const float x4 = gam::scl::wrap(normalizedPhasor * 4.0f);

        outputs[REVERSE_OUTPUT].setVoltage(reversed * HCV_PHZ_UPSCALE, i);
        outputs[PINGPONG_OUTPUT].setVoltage(pingPong * HCV_PHZ_UPSCALE, i);
        outputs[PONGPING_OUTPUT].setVoltage(pongPing * HCV_PHZ_UPSCALE, i);
        outputs[MULT2_OUTPUT].setVoltage(x2 * HCV_PHZ_UPSCALE, i);
        outputs[MULT4_OUTPUT].setVoltage(x4 * HCV_PHZ_UPSCALE, i);
    }
    
}

struct PhasorGeometryWidget : HCVModuleWidget { PhasorGeometryWidget(PhasorGeometry *module); };

PhasorGeometryWidget::PhasorGeometryWidget(PhasorGeometry *module)
{
    setSkinPath("res/PhasorGeometry.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorGeometry::PHASOR_INPUT);

    for(int i = 0; i < PhasorGeometry::NUM_OUTPUTS; i++)
    {
        const int yPos = i*42;
        createOutputPort(33, 115 + yPos, i);
    }
}

Model *modelPhasorGeometry = createModel<PhasorGeometry, PhasorGeometryWidget>("PhasorGeometry");
