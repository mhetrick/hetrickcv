#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorSwing : HCVModule
{
	enum ParamIds
	{
		STEPS_PARAM, STEPS_SCALE_PARAM,
        GROUPING_PARAM, GROUPING_SCALE_PARAM,
        SWING_PARAM, SWING_SCALE_PARAM,
        VARIATION_PARAM, VARIATION_SCALE_PARAM,
        MODE_PARAM, MODE_SCALE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,

        STEPS_INPUT,
        GROUPING_INPUT,
        SWING_INPUT,
        VARIATION_INPUT,
        MODE_INPUT,

        ACTIVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		SWING_OUTPUT,
        PHASORS_OUTPUT,
        GATES_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        ENUMS(MODE_LIGHTS, 6),

        ACTIVE_LIGHT,
        OUT_LIGHT,
        PHASORS_LIGHT,
        GATES_LIGHT,
        
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

	PhasorSwing()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, SWING_OUTPUT);

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 1.0, "Steps");
		configParam(STEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(GROUPING_PARAM, 1.0, MAX_STEPS, 2.0, "Grouping");
		configParam(GROUPING_SCALE_PARAM, -1.0, 1.0, 1.0, "Grouping CV Depth");
        paramQuantities[GROUPING_PARAM]->snapEnabled = true;

        configParam(SWING_PARAM, -5.0, 5.0, 0.0, "Swing");
		configParam(SWING_SCALE_PARAM, -1.0, 1.0, 1.0, "Swing CV Depth");

        configParam(VARIATION_PARAM, 0.0, 5.0, 0.0, "Variation");
		configParam(VARIATION_SCALE_PARAM, -1.0, 1.0, 1.0, "Variation CV Depth");
        
        configSwitch(PhasorSwing::MODE_PARAM, 0.0, 5.0, 0.0, "Mode",
        {"Random Slice", "Random Reverse Slice", "Random Reverse Phasor", "Random Slope", "Random Stutter", "Random Freeze"});
        paramQuantities[MODE_PARAM]->snapEnabled = true;
		configParam(PhasorSwing::MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Mode CV Depth");


        configInput(PHASOR_INPUT, "Phasor");

        configInput(STEPS_INPUT, "Steps CV");
        configInput(GROUPING_INPUT, "Grouping CV");
        configInput(VARIATION_INPUT, "Variation CV");
        configInput(SWING_INPUT, "Swing CV");
        configInput(MODE_INPUT, "Mode CV");
        configInput(ACTIVE_INPUT, "Activation Gate");

        configOutput(SWING_OUTPUT, "Swung Phasor");
        configOutput(PHASORS_OUTPUT, "Step Phasors");
        configOutput(GATES_OUTPUT, "Step Gates");
	}

	void process(const ProcessArgs &args) override;

    HCVPhasorSwingProcessor swingProcs[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorSwing::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPS_SCALE_PARAM].getValue() * STEPS_CV_SCALE;

    float groupingKnob = params[GROUPING_PARAM].getValue();
    float groupingCVDepth = params[GROUPING_SCALE_PARAM].getValue() * STEPS_CV_SCALE;

    float swingKnob = params[SWING_PARAM].getValue();
    float swingCVDepth = params[SWING_SCALE_PARAM].getValue();

    float variationKnob = params[VARIATION_PARAM].getValue();
    float variationCVDepth = params[VARIATION_SCALE_PARAM].getValue();

    float modeKnob = params[MODE_PARAM].getValue();
    float modeCVDepth = params[MODE_SCALE_PARAM].getValue();

    for(int i = 0; i < numChannels; i++)
    {
        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        float swing = swingKnob + (swingCVDepth * inputs[SWING_INPUT].getPolyVoltage(i));
        swing = clamp(swing, -5.0f, 5.0f) * 0.2f;

        float variation = variationKnob + (variationCVDepth * inputs[VARIATION_INPUT].getPolyVoltage(i));
        variation = clamp(variation, 0.0f, 5.0f) * 0.2f;

        float steps = stepsKnob + (stepsCVDepth * inputs[STEPS_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));

        float grouping = groupingKnob + (groupingCVDepth * inputs[GROUPING_INPUT].getPolyVoltage(i));
        grouping = floorf(clamp(grouping, 1.0f, MAX_STEPS));

        float mode = modeKnob + (modeCVDepth * inputs[MODE_INPUT].getPolyVoltage(i));
        mode = floorf(clamp(mode, 0.0f, 5.0f));

        bool active = true;
        if(inputs[ACTIVE_INPUT].isConnected())
        {
            active = inputs[ACTIVE_INPUT].getPolyVoltage(i) >= 1.0f;
        }

        swingProcs[i].setNumStepsAndGrouping(steps, grouping);
        swingProcs[i].setSwing(swing);
        swingProcs[i].setVariation(variation);

        float output = normalizedPhasor;
        if(active)
        {
            output = swingProcs[i](normalizedPhasor);
        }

        const float stepPhasor = swingProcs[i].getStepPhasorOutput();

        outputs[SWING_OUTPUT].setVoltage(output * HCV_PHZ_UPSCALE, i);
        outputs[PHASORS_OUTPUT].setVoltage(stepPhasor * HCV_PHZ_UPSCALE, i);
        outputs[GATES_OUTPUT].setVoltage(stepPhasor < 0.5f ? HCV_PHZ_GATESCALE : 0.0f, i);
    }

    int lightMode = modeKnob + modeCVDepth*inputs[MODE_INPUT].getVoltage();
    lightMode = clamp(lightMode, 0, 5);

    for(int i = 0; i < 6; i++)
    {
        lights[i].setBrightness(i == lightMode ? 5.0f : 0.0f);
    }

    bool active = true;
    if(inputs[ACTIVE_INPUT].isConnected())
    {
        active = inputs[ACTIVE_INPUT].getVoltage() >= 1.0f;
    }

    lights[ACTIVE_LIGHT].setBrightness(active ? 1.0f : 0.0f);
    

    setLightFromOutput(OUT_LIGHT, SWING_OUTPUT);
    setLightFromOutput(PHASORS_LIGHT, PHASORS_OUTPUT);
    setLightSmoothFromOutput(GATES_LIGHT, GATES_OUTPUT);
}


struct PhasorSwingWidget : HCVModuleWidget { PhasorSwingWidget(PhasorSwing *module); };

PhasorSwingWidget::PhasorSwingWidget(PhasorSwing *module)
{
	setSkinPath("res/PhasorSwing.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 40.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorSwing::STEPS_PARAM, PhasorSwing::STEPS_SCALE_PARAM, PhasorSwing::STEPS_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorSwing::GROUPING_PARAM, PhasorSwing::GROUPING_SCALE_PARAM, PhasorSwing::GROUPING_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorSwing::SWING_PARAM, PhasorSwing::SWING_SCALE_PARAM, PhasorSwing::SWING_INPUT);
    createParamComboHorizontal(knobX, knobY + 150, PhasorSwing::VARIATION_PARAM, PhasorSwing::VARIATION_SCALE_PARAM, PhasorSwing::VARIATION_INPUT);
    //createParamComboHorizontal(knobX, knobY + 200, PhasorSwing::MODE_PARAM, PhasorSwing::MODE_SCALE_PARAM, PhasorSwing::MODE_INPUT);

    const float inJackY = 258.0f;
    const float outJackY = 318.0f;
    const float jack1 = 15.0f;
    const float jack2 = 78.0f;
    const float jack3 = 140.0f;
    //const float activeX = 60.0f;
	//////INPUTS//////
    createInputPort(jack1, inJackY, PhasorSwing::PHASOR_INPUT);
    createInputPort(jack2, inJackY, PhasorSwing::ACTIVE_INPUT);

	//////OUTPUTS//////
    createOutputPort(jack1, outJackY, PhasorSwing::SWING_OUTPUT);
    createOutputPort(jack2, outJackY, PhasorSwing::PHASORS_OUTPUT);
    createOutputPort(jack3, outJackY, PhasorSwing::GATES_OUTPUT);


    for (int i = 0; i < 6; i++)
    {
        //createHCVRedLight(100.0, 238 + (i*9.5), i);
    }

    createHCVRedLightForJack(jack2, inJackY, PhasorSwing::ACTIVE_LIGHT);
    
    createHCVRedLightForJack(jack1, outJackY, PhasorSwing::OUT_LIGHT);
    createHCVRedLightForJack(jack2, outJackY, PhasorSwing::PHASORS_LIGHT);
    createHCVRedLightForJack(jack3, outJackY, PhasorSwing::GATES_LIGHT);
    
}

Model *modelPhasorSwing = createModel<PhasorSwing, PhasorSwingWidget>("PhasorSwing");
