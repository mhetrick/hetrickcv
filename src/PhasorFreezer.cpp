#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"
#include "dsp/digital.hpp"

struct PhasorFreezer : HCVModule
{
	enum ParamIds
	{
		PHASE_PARAM,
        PHASECV_PARAM,
        RANGE_PARAM,
        FREEZE_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        PHASECV_INPUT,
        FREEZE_INPUT,
        RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds
    {
        PHASOR_LIGHT,
        NUM_LIGHTS
	};

	PhasorFreezer()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(PHASE_PARAM, -5.0f, 5.0f, 0.0f, "Reset Phase");
		configParam(PHASECV_PARAM, -1.0f, 1.0f, 1.0f, "Reset Phase CV Depth");
        configButton(RESET_PARAM, "Reset");
        configButton(FREEZE_PARAM, "Freeze");

		configInput(PHASECV_INPUT, "Reset Phase CV");

        configInput(FREEZE_INPUT, "Freeze");
        configInput(RESET_INPUT, "Reset");

		configInput(MAIN_INPUT, "Phasor");
		configOutput(MAIN_OUTPUT, "Phasor");
	}

    HCVPhasorFreezer freezers[16];
    dsp::SchmittTrigger inputTriggers[16];

	void process(const ProcessArgs &args) override;
};


void PhasorFreezer::process(const ProcessArgs &args)
{
	const float shiftKnob = params[PHASE_PARAM].getValue();
	const float shiftScale = params[PHASECV_INPUT].getValue();
    const float resetButton = params[RESET_PARAM].getValue();
    const float freezeButton = params[FREEZE_PARAM].getValue();

	int numChannels = setupPolyphonyForAllOutputs();

	for (int i = 0; i < numChannels; i++) 
	{
        float resetPhase = shiftKnob + shiftScale * inputs[PHASECV_INPUT].getPolyVoltage(i);
        resetPhase = clamp(resetPhase, -5.0f, 5.0f) * 0.2f;

        float normalizedPhasor = scaleAndWrapPhasor(inputs[MAIN_INPUT].getPolyVoltage(i));
        float resetInputs = inputs[RESET_INPUT].getPolyVoltage(i) + resetButton;
        bool resetTrigger = inputTriggers[i].process(resetInputs);

        bool frozen = (inputs[FREEZE_INPUT].getPolyVoltage(i) + freezeButton);

        if(resetTrigger) freezers[i].reset(resetPhase);

        float output = freezers[i](normalizedPhasor, frozen);
        outputs[MAIN_OUTPUT].setVoltage(output * HCV_PHZ_UPSCALE, i);
	}

	setLightFromOutput(PHASOR_LIGHT, MAIN_OUTPUT);
}


struct PhasorFreezerWidget : HCVModuleWidget { PhasorFreezerWidget(PhasorFreezer *module); };

PhasorFreezerWidget::PhasorFreezerWidget(PhasorFreezer *module)
{
	setSkinPath("res/PhasorFreezer.svg");
	initializeWidget(module);

	//////PARAMS//////
    createParamComboVertical(27, 100, PhasorFreezer::PHASE_PARAM, PhasorFreezer::PHASECV_PARAM, PhasorFreezer::PHASECV_INPUT);

	//////INPUTS//////
    int inputY = 248;
	createInputPort(13, inputY, PhasorFreezer::FREEZE_INPUT);
	createInputPort(53, inputY, PhasorFreezer::RESET_INPUT);

    createHCVButtonSmallForJack(13, inputY, PhasorFreezer::FREEZE_PARAM);
    createHCVButtonSmallForJack(53, inputY, PhasorFreezer::RESET_PARAM);

	//////OUTPUTS//////
    int bottomY = 300;
    createInputPort(13, bottomY, PhasorFreezer::MAIN_INPUT);
	createOutputPort(53, bottomY, PhasorFreezer::MAIN_OUTPUT);

	createHCVRedLightForJack(53, bottomY, PhasorFreezer::PHASOR_LIGHT);
}

Model *modelPhasorFreezer = createModel<PhasorFreezer, PhasorFreezerWidget>("PhasorFreezer");
