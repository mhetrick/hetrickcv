#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DSP/HCVTiming.h"

struct PhasorAnalyzer : HCVModule
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
        DIRECTION_OUTPUT,
        ACTIVE_OUTPUT,
        RESET_OUTPUT,
        JUMP_OUTPUT,
        KINK_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        DIRECTION_POS_LIGHT,
        DIRECTION_NEG_LIGHT,
        ACTIVE_LIGHT,
        RESET_LIGHT,
        JUMP_LIGHT,
        KINK_LIGHT,
        NUM_LIGHTS
	};


	PhasorAnalyzer()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(PHASOR_INPUT, "Phasor");
        
        configOutput(DIRECTION_OUTPUT, "Direction");
        configOutput(ACTIVE_OUTPUT, "Active Gate");

        configOutput(RESET_OUTPUT, "Reset Trigger");
        configOutput(JUMP_OUTPUT, "Jump Trigger");
        configOutput(KINK_OUTPUT, "Kink Trigger");
	}

	void process(const ProcessArgs &args) override;

    HCVPhasorResetDetector resetDetectors[16];
    HCVPhasorResetDetector jumpDetectors[16];
    HCVPhasorSlopeDetector slopeDetectors[16];
    HCVPhasorSlopeDetector kinkDetectors[16];
    HCVTriggeredGate resetTriggers[16];
    HCVTriggeredGate jumpTriggers[16];

    float lastSlope[16] = {};

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorAnalyzer::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);
        const float slope = slopeDetectors[i](normalizedPhasor);
        const bool reset = resetDetectors[i].detectSimpleReset(normalizedPhasor);
        const bool jump = resetDetectors[i].detectProportionalReset(normalizedPhasor);
        const bool active = slopeDetectors[i].isPhasorAdvancing();

        const bool resetTrigger = resetTriggers[i].process(reset);
        const bool jumpTrigger = jumpTriggers[i].process(reset);

        const float rawSlope = kinkDetectors[i].calculateRawSlope(normalizedPhasor);
        const float kink = clamp((rawSlope - lastSlope[i]) * 5.0f, -5.0f, 5.0f);
        lastSlope[i] = rawSlope;

        float direction = 0.0f;
        if(slope > 0.0f) direction = 5.0f;
        else if (slope < 0.0f) direction = -5.0f;

        outputs[DIRECTION_OUTPUT].setVoltage(direction, i);
        outputs[ACTIVE_OUTPUT].setVoltage(active ? HCV_PHZ_GATESCALE : 0.0f, i);
        outputs[RESET_OUTPUT].setVoltage(resetTrigger ? HCV_PHZ_GATESCALE : 0.0f, i);
        outputs[JUMP_OUTPUT].setVoltage(jumpTrigger ? HCV_PHZ_GATESCALE : 0.0f, i);
        outputs[KINK_OUTPUT].setVoltage(kink, i);
    }
    
    setBipolarLightBrightness(DIRECTION_POS_LIGHT, outputs[DIRECTION_OUTPUT].getVoltage() * 0.2f);
    lights[ACTIVE_LIGHT].setBrightness(outputs[ACTIVE_OUTPUT].getVoltage() * 0.1f);
    lights[RESET_LIGHT].setBrightnessSmooth(outputs[RESET_OUTPUT].getVoltage() * 0.1f, args.sampleTime * 20.0f);
    lights[JUMP_LIGHT].setBrightnessSmooth(outputs[JUMP_OUTPUT].getVoltage() * 0.1f, args.sampleTime * 20.0f);
    lights[KINK_LIGHT].setBrightnessSmooth(outputs[KINK_OUTPUT].getVoltage() * 0.2f, args.sampleTime * 20.0f);
}

struct PhasorAnalyzerWidget : HCVModuleWidget { PhasorAnalyzerWidget(PhasorAnalyzer *module); };

PhasorAnalyzerWidget::PhasorAnalyzerWidget(PhasorAnalyzer *module)
{
    setSkinPath("res/PhasorAnalyzer.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorAnalyzer::PHASOR_INPUT);

    for(int i = 0; i < PhasorAnalyzer::NUM_OUTPUTS; i++)
    {
        const int yPos = i*42;
        createOutputPort(33, 115 + yPos, i);

        if(i == 0) createHCVBipolarLight(28, 113 + yPos, PhasorAnalyzer::DIRECTION_POS_LIGHT);
        else createHCVRedLight(28, 113 + yPos, PhasorAnalyzer::DIRECTION_NEG_LIGHT + i);
    }
}

Model *modelPhasorAnalyzer = createModel<PhasorAnalyzer, PhasorAnalyzerWidget>("PhasorAnalyzer");
