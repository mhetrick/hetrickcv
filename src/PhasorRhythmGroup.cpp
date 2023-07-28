#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"
#include "DSP/HCVTiming.h"

struct PhasorRhythmGroup : HCVModule
{
    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

	enum ParamIds
	{
		STEPS_PARAM, STEPS_SCALE_PARAM,
        GROUPSTEPS_PARAM, GROUPSTEPS_SCALE_PARAM,
        SUBGROUPSTEPS_PARAM, SUBGROUPSTEPS_SCALE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,

        STEPS_INPUT,
        GROUPSTEPS_INPUT,
        SUBGROUPSTEPS_INPUT,

		NUM_INPUTS
	};
	enum OutputIds
	{
		MAINPHASOR_OUTPUT,
        MAINTRIG_OUTPUT,
        GROUPPHASOR_OUTPUT,
        GROUPTRIG_OUTPUT,
        SUBGROUPPHASOR_OUTPUT,
        SUBGROUPTRIG_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        MAINTRIG_LIGHT,
        GROUPTRIG_LIGHT,
        SUBGROUPTRIG_LIGHT,
        NUM_LIGHTS
	};

	PhasorRhythmGroup()
	{

		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);


		configParam(STEPS_PARAM, 1.0f, MAX_STEPS, 1.0f, "Steps");
		configParam(STEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");

        configParam(GROUPSTEPS_PARAM, 1.0f, MAX_STEPS, 1.0, "Group Steps");
		configParam(GROUPSTEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Group Steps CV Depth");

        configParam(SUBGROUPSTEPS_PARAM, 1.0f, MAX_STEPS, 1.0, "Subgroup Steps");
		configParam(SUBGROUPSTEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Subgroup Steps CV Depth");


        paramQuantities[STEPS_PARAM]->snapEnabled = true;
        paramQuantities[GROUPSTEPS_PARAM]->snapEnabled = true;
        paramQuantities[SUBGROUPSTEPS_PARAM]->snapEnabled = true;
        

        configInput(PHASOR_INPUT, "Phasor");

        configInput(STEPS_INPUT, "Steps CV");
        configInput(GROUPSTEPS_INPUT, "Group Steps CV");
        configInput(SUBGROUPSTEPS_INPUT, "Subgroup Steps CV");
        
        configOutput(MAINPHASOR_OUTPUT, "Steps Phasors");
        configOutput(MAINTRIG_OUTPUT, "Steps Triggers");
        configOutput(GROUPPHASOR_OUTPUT, "Group Steps Phasors");
        configOutput(GROUPTRIG_OUTPUT, "Group Steps Triggers");
        configOutput(SUBGROUPPHASOR_OUTPUT, "Subgroup Steps Phasors");
        configOutput(SUBGROUPTRIG_OUTPUT, "Subgroup Steps Triggers");

        for (int i = 0; i < 16; i++)
        {
            currentGroupSteps[i] = 1.0f;
            currentSubgroupSteps[i] = 1.0f;
        }
        
	}

	void process(const ProcessArgs &args) override;

    HCVPhasorStepDetector stepDetectors[16];
    HCVPhasorResetDetector mainResetDetectors[16];
    HCVPhasorResetDetector groupResetDetectors[16];
    HCVPhasorResetDetector subgroupResetDetectors[16];
    HCVTriggeredGate mainTrigs[16];
    HCVTriggeredGate groupTrigs[16];
    HCVTriggeredGate subgroupTrigs[16];

    float currentGroupSteps[16];
    float currentSubgroupSteps[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorRhythmGroup::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float groupStepsKnob = params[GROUPSTEPS_PARAM].getValue();
    float subgroupStepsKnob = params[SUBGROUPSTEPS_PARAM].getValue();

    float stepsCVDepth = params[STEPS_SCALE_PARAM].getValue() * STEPS_CV_SCALE;
    float groupStepsCVDepth = params[GROUPSTEPS_SCALE_PARAM].getValue() * STEPS_CV_SCALE;
    float subgroupStepsCVDepth = params[SUBGROUPSTEPS_SCALE_PARAM].getValue() * STEPS_CV_SCALE;

    for (int i = 0; i < numChannels; i++)
    {
        float steps = stepsKnob + (stepsCVDepth * inputs[STEPS_INPUT].getPolyVoltage(i));
        steps = clamp(steps, 1.0f, MAX_STEPS);
        stepDetectors[i].setNumberSteps(steps);

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        const bool mainStepTriggered = stepDetectors[i](normalizedPhasor);
        if(mainStepTriggered)   //quantize rhythm changes to main clock
        {
            float groupSteps = groupStepsKnob + (groupStepsCVDepth * inputs[GROUPSTEPS_INPUT].getPolyVoltage(i));
            groupSteps = clamp(groupSteps, 1.0f, MAX_STEPS);

            float subgroupSteps = subgroupStepsKnob + (subgroupStepsCVDepth * inputs[SUBGROUPSTEPS_INPUT].getPolyVoltage(i));
            subgroupSteps = clamp(subgroupSteps, 1.0f, MAX_STEPS);

            currentGroupSteps[i] = std::max(groupSteps, subgroupSteps);
            currentSubgroupSteps[i] = std::min(groupSteps, subgroupSteps);
        }

        const float mainStepPhasor = stepDetectors[i].getFractionalStep();
        const float mainStepTrig = mainTrigs[i].process(mainStepTriggered) ? HCV_GATE_MAG : 0.0f;

        float groupPhasor = normalizedPhasor * steps;
        groupPhasor = gam::scl::wrap(groupPhasor, currentGroupSteps[i], 0.0f);
        const float normalizedGroupPhasor = groupPhasor/currentGroupSteps[i];

        bool groupTriggered = groupResetDetectors[i](normalizedGroupPhasor);
        float groupTrig = groupTrigs[i].process(groupTriggered) && mainStepTrig ? HCV_GATE_MAG : 0.0f;

        const float subgroupPhasor = gam::scl::wrap(groupPhasor, currentSubgroupSteps[i], 0.0f);
        const float normalizedSubgroupPhasor = subgroupPhasor/currentSubgroupSteps[i];

        bool subgroupTriggered = subgroupResetDetectors[i](normalizedSubgroupPhasor);
        float subgroupTrig = subgroupTrigs[i].process(subgroupTriggered) && mainStepTrig ? HCV_GATE_MAG : 0.0f;

        outputs[MAINPHASOR_OUTPUT].setVoltage(mainStepPhasor * HCV_PHZ_UPSCALE, i);
        outputs[MAINTRIG_OUTPUT].setVoltage(mainStepTrig, i);

        outputs[GROUPPHASOR_OUTPUT].setVoltage(normalizedGroupPhasor * HCV_PHZ_UPSCALE, i);
        outputs[GROUPTRIG_OUTPUT].setVoltage(groupTrig, i);

        outputs[SUBGROUPPHASOR_OUTPUT].setVoltage(normalizedSubgroupPhasor * HCV_PHZ_UPSCALE, i);
        outputs[SUBGROUPTRIG_OUTPUT].setVoltage(subgroupTrig, i);
    }

    lights[MAINTRIG_LIGHT].setBrightnessSmooth(outputs[MAINTRIG_OUTPUT].getVoltage(), args.sampleTime * 10.0f);
    lights[GROUPTRIG_LIGHT].setBrightnessSmooth(outputs[GROUPTRIG_OUTPUT].getVoltage(), args.sampleTime * 10.0f);
    lights[SUBGROUPTRIG_LIGHT].setBrightnessSmooth(outputs[SUBGROUPTRIG_OUTPUT].getVoltage(), args.sampleTime * 10.0f);
}


struct PhasorRhythmGroupWidget : HCVModuleWidget { PhasorRhythmGroupWidget(PhasorRhythmGroup *module); };

PhasorRhythmGroupWidget::PhasorRhythmGroupWidget(PhasorRhythmGroup *module)
{
	setSkinPath("res/PhasorRhythmGroup.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorRhythmGroup::STEPS_PARAM, PhasorRhythmGroup::STEPS_SCALE_PARAM, PhasorRhythmGroup::STEPS_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorRhythmGroup::GROUPSTEPS_PARAM, PhasorRhythmGroup::GROUPSTEPS_SCALE_PARAM, PhasorRhythmGroup::GROUPSTEPS_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorRhythmGroup::SUBGROUPSTEPS_PARAM, PhasorRhythmGroup::SUBGROUPSTEPS_SCALE_PARAM, PhasorRhythmGroup::SUBGROUPSTEPS_INPUT);


    const float jackY1 = 235.0f;
    const float jackY2 = 305.0f;
    const float lightOffsetX = -5.0f;
    const float lightOffsetY = -2.0f;

    float xSpacing = 43.0f;
    float jackX1 = 13.0f;
    float jackX2 = jackX1 + xSpacing - 1;
    float jackX3 = jackX2 + xSpacing;
    float jackX4 = jackX3 + xSpacing - 2;

	//////INPUTS//////
    createInputPort(30.0f, jackY1, PhasorRhythmGroup::PHASOR_INPUT);

	//////OUTPUTS//////
    createOutputPort(jackX3, jackY1, PhasorRhythmGroup::MAINPHASOR_OUTPUT);
    createOutputPort(jackX4, jackY1, PhasorRhythmGroup::MAINTRIG_OUTPUT);
    createHCVRedLight(jackX4 + lightOffsetX, jackY1 + lightOffsetY, PhasorRhythmGroup::MAINTRIG_LIGHT);

    createOutputPort(jackX1, jackY2, PhasorRhythmGroup::GROUPPHASOR_OUTPUT);
    createOutputPort(jackX2, jackY2, PhasorRhythmGroup::GROUPTRIG_OUTPUT);
    createHCVRedLight(jackX2 + lightOffsetX, jackY2 + lightOffsetY, PhasorRhythmGroup::GROUPTRIG_LIGHT);

    createOutputPort(jackX3, jackY2, PhasorRhythmGroup::SUBGROUPPHASOR_OUTPUT);
    createOutputPort(jackX4, jackY2, PhasorRhythmGroup::SUBGROUPTRIG_OUTPUT);
    createHCVRedLight(jackX4 + lightOffsetX, jackY2 + lightOffsetY, PhasorRhythmGroup::SUBGROUPTRIG_LIGHT);
    
}

Model *modelPhasorRhythmGroup = createModel<PhasorRhythmGroup, PhasorRhythmGroupWidget>("PhasorRhythmGroup");
