#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"
#include "dsp/digital.hpp"

struct PhasorReset : HCVModule
{
	enum ParamIds
	{
		PHASE_PARAM,
        PHASECV_PARAM,
        RANGE_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        PHASECV_INPUT,
        RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
		NUM_OUTPUTS
	};

	PhasorReset()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

		configBypass(MAIN_INPUT, MAIN_OUTPUT);

		configParam(PHASE_PARAM, -5.0f, 5.0f, 0.0f, "Reset Phase");
		configParam(PHASECV_PARAM, -1.0f, 1.0f, 1.0f, "Reset Phase CV Depth");
        configButton(RESET_PARAM, "Reset");

		configInput(PHASECV_INPUT, "Reset Phase CV");
		configInput(MAIN_INPUT, "Phasor");
		configOutput(MAIN_OUTPUT, "Phasor");
	}

    HCVPhasorDivMult divmults[16];
    dsp::SchmittTrigger inputTriggers[16];

	void process(const ProcessArgs &args) override;
};


void PhasorReset::process(const ProcessArgs &args)
{
	const float shiftKnob = params[PHASE_PARAM].getValue();
	const float shiftScale = params[PHASECV_INPUT].getValue();
    const float resetButton = params[RESET_PARAM].getValue();

	int numChannels = setupPolyphonyForAllOutputs();

	for (int i = 0; i < numChannels; i++) 
	{
        float resetPhase = shiftKnob + shiftScale * inputs[PHASECV_INPUT].getPolyVoltage(i);
        resetPhase = clamp(resetPhase, -5.0f, 5.0f) * 0.2f;

        float normalizedPhasor = scaleAndWrapPhasor(inputs[MAIN_INPUT].getPolyVoltage(i));
        float resetInputs = inputs[RESET_INPUT].getPolyVoltage(i) + resetButton;
        bool resetTrigger = inputTriggers[i].process(resetInputs);

        if(resetTrigger) divmults[i].reset(resetPhase);

        float output = divmults[i].basicSync(normalizedPhasor);
        outputs[MAIN_OUTPUT].setVoltage(output * HCV_PHZ_UPSCALE, i);
	}
}


struct PhasorResetWidget : HCVModuleWidget { PhasorResetWidget(PhasorReset *module); };

PhasorResetWidget::PhasorResetWidget(PhasorReset *module)
{
	setSkinPath("res/PhasorReset.svg");
	initializeWidget(module);

	//////PARAMS//////
    createParamComboVertical(27, 100, PhasorReset::PHASE_PARAM, PhasorReset::PHASECV_PARAM, PhasorReset::PHASECV_INPUT);
    createHCVButtonSmall(57, 230, PhasorReset::RESET_PARAM);

	//////INPUTS//////
    int inputY = 248;
    addInput(createInput<PJ301MPort>(Vec(13, inputY), module, PhasorReset::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(53, inputY), module, PhasorReset::RESET_INPUT));

	//////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 300), module, PhasorReset::MAIN_OUTPUT));
}

Model *modelPhasorReset = createModel<PhasorReset, PhasorResetWidget>("PhasorReset");
