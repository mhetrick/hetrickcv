#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DSP/HCVTiming.h"

struct PhasorSplitter : HCVModule
{
    static constexpr int NUM_STEPS = 64;
    static constexpr float STEPS_CV_SCALE = (NUM_STEPS - 1)/5.0f;

	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        MODE_PARAM,
        MODECV_PARAM,
        DETECTION_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        MODECV_INPUT,
        RUN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        ENUMS(PHASOR_OUTPUTS, NUM_STEPS),
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ENUMS(STEP_LIGHTS, NUM_STEPS*2),
        RUN_LIGHT,
        NUM_LIGHTS
	};

    bool smartDetection = true;
    int currentStep[16] = {0};

    HCVPhasorSlopeDetector slopeDetectors[16];
    HCVPhasorStepDetector stepDetectors[16];
    HCVTriggeredGate triggers[16];

	PhasorSplitter()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(STEPS_PARAM, 1.0, NUM_STEPS, 16.0, "Steps");
        configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");

        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(MODE_PARAM, -5.0, 5.0, 0.0, "Gate Width");
        configParam(MODECV_PARAM, -1.0, 1.0, 1.0, "Gate Width CV Depth");

        configSwitch(DETECTION_PARAM, 0.0, 1.0, 1.0, "Detection Mode", {"Raw", "Smart (Detect Playback and Reverse)"});

        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(MODECV_INPUT, "Gate Width CV");
        configInput(PHASOR_INPUT, "Phasor CV");

        for (int i = 0; i < NUM_STEPS; i++) 
        {
			configOutput(PHASOR_OUTPUTS + i, string::f("Output %d", i + 1));
		}

		onReset();
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorSplitter::process(const ProcessArgs &args)
{
    const float stepsKnob = params[STEPS_PARAM].getValue();
    const float stepsDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float modeKnob = params[MODE_PARAM].getValue();
    const float modeDepth = params[MODECV_PARAM].getValue();

    int numChannels = setupPolyphonyForAllOutputs();
    int lightIndex = 0;

    smartDetection = params[DETECTION_PARAM].getValue() > 0.0f;

    for (int chan = 0; chan < numChannels; chan++)
    {
        float numSteps = stepsKnob + (stepsDepth * inputs[STEPSCV_INPUT].getPolyVoltage(chan));
        numSteps = clamp(numSteps, 1.0f, float(NUM_STEPS));

        bool active = true;
        if(inputs[RUN_INPUT].isConnected())
        {
            active = inputs[RUN_INPUT].getPolyVoltage(chan) >= 1.0f;
        }

        const float phasorIn = active ? inputs[PHASOR_INPUT].getPolyVoltage(chan) : 0.0f;
        float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        stepDetectors[chan].setNumberSteps(numSteps);
        bool triggered = stepDetectors[chan](normalizedPhasor);
        currentStep[chan] = stepDetectors[chan].getCurrentStep();
        float fractionalIndex = stepDetectors[chan].getFractionalStep();

        for (int i = 0; i < NUM_STEPS; i++)
        {
            outputs[PHASOR_OUTPUTS + i].setVoltage(0.0f, chan);
        }
        
        outputs[PHASOR_OUTPUTS + currentStep[chan]].setVoltage(fractionalIndex * HCV_PHZ_UPSCALE, chan);
    }

    bool active = true;
    if(inputs[RUN_INPUT].isConnected())
    {
        active = inputs[RUN_INPUT].getPolyVoltage(0) >= 1.0f;
    }
    bool isPlaying = slopeDetectors[0].isPhasorAdvancing() && active;

    // Step Lights
    for (int i = 0; i < NUM_STEPS; i++) 
    {
        lights[STEP_LIGHTS + 2 * i + 0].setBrightness(0.0); //green
        lights[STEP_LIGHTS + 2 * i + 1].setBrightness(i >= stepsKnob); //red
    }

    const float currentOutputValue = getOutput(PHASOR_OUTPUTS + currentStep[0]).getVoltage() * HCV_PHZ_DOWNSCALE;
    lights[STEP_LIGHTS + 2 * currentStep[0]].setBrightness(currentOutputValue);

    lights[RUN_LIGHT].setBrightness(active ? 1.0f : 0.0f);
}

struct PhasorSplitterWidget : HCVModuleWidget { PhasorSplitterWidget(PhasorSplitter *module); };

PhasorSplitterWidget::PhasorSplitterWidget(PhasorSplitter *module)
{
    setSkinPath("res/PhasorSplitter.svg");
    initializeWidget(module);
    
    //////PARAMS//////
    int knobY = 60;
    createParamComboVertical(15, knobY, PhasorSplitter::MODE_PARAM, PhasorSplitter::MODECV_PARAM, PhasorSplitter::MODECV_INPUT);
    createParamComboVertical(70, knobY, PhasorSplitter::STEPS_PARAM, PhasorSplitter::STEPSCV_PARAM, PhasorSplitter::STEPSCV_INPUT);
    
    createHCVSwitchVert(53, 208, PhasorSplitter::DETECTION_PARAM);

    //////INPUTS//////
    int jackX = 20;
    int jackX2 = 78;
    int inputY = 320;
    createInputPort(jackX, inputY, PhasorSplitter::PHASOR_INPUT);
    createInputPort(jackX2, inputY, PhasorSplitter::RUN_INPUT);

    //////OUTPUTS/////
    int outputY = 310;

    for (int i = 0; i < PhasorSplitter::NUM_STEPS; i++)
    {
        int outJackX = 115 + 30 * (i%8);
        int outJackY = 45 + (i/8)*41;

        createOutputPort(outJackX, outJackY, PhasorSplitter::PHASOR_OUTPUTS + i);
        createHCVBipolarLightForJack(outJackX, outJackY, PhasorSplitter::STEP_LIGHTS + 2*i);
    }

    createHCVGreenLightForJack(jackX2, inputY, PhasorSplitter::RUN_LIGHT);
}

Model *modelPhasorSplitter = createModel<PhasorSplitter, PhasorSplitterWidget>("PhasorSplitter");
