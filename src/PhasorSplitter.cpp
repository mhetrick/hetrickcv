#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"
#include "DSP/HCVTiming.h"
#include "DSP/Phasors/HCVPhasorEffects.h"

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
        RESET_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        MODECV_INPUT,
        RUN_INPUT,
        RESET_INPUT,
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
        ENUMS(MODE_LIGHTS, 5),
        RUN_LIGHT,
        NUM_LIGHTS
	};

    bool smartDetection = true;
    int currentStep[16] = {0};

    HCVPhasorSlopeDetector slopeDetectors[16];
    HCVPhasorStepDetector stepDetectors[16];
    HCVPhasorResetDetector resetDetectors[16];
    HCVPhasorDivMult divMults[16];
    dsp::SchmittTrigger resetTrigger[16];

	PhasorSplitter()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(STEPS_PARAM, 1.0, NUM_STEPS, 16.0, "Steps");
        configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        configButton(RESET_PARAM, "Reset");

        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configSwitch(MODE_PARAM, 0.0, 4.0, 1.0, "Split Mode", 
        {"Scan", "Divide + Scan", "Step Forwards", "Step Backwards", "Step Random"});
        configParam(MODECV_PARAM, -1.0, 1.0, 1.0, "Gate Width CV Depth");

        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(MODECV_INPUT, "Gate Width CV");
        configInput(PHASOR_INPUT, "Phasor CV");
        configInput(RESET_INPUT, "Reset");

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

    const float resetButton = params[RESET_PARAM].getValue();

    int numChannels = setupPolyphonyForAllOutputs();
    int lightIndex = 0;

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
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        float modeMod = modeKnob + (modeDepth * inputs[MODECV_INPUT].getPolyVoltage(chan));
        int currentMode = (int)clamp(modeMod, 0.0f, 4.0f);

        float outputValue = 0.0f;

        if(resetTrigger[chan].process(inputs[RESET_INPUT].getPolyVoltage(chan) + resetButton))
        {
            divMults[chan].reset();
            currentStep[chan] = 0;
        }

        if(currentMode < 2)
        {
            float division = currentMode == 0 ? 1 : numSteps;
            divMults[chan].setDivider(division);
            float dividedPhasor = divMults[chan].basicSync(normalizedPhasor);

            stepDetectors[chan].setNumberSteps(numSteps);
            const bool triggered = stepDetectors[chan](dividedPhasor);

            currentStep[chan] = stepDetectors[chan].getCurrentStep();
            outputValue = stepDetectors[chan].getFractionalStep();
        }
        else
        {
            const bool resetDetected = resetDetectors[chan](normalizedPhasor);
            if(resetDetected) 
            {
                currentStep[chan] = currentStep[chan] % int(numSteps);

                switch (currentMode)
                {
                case 2:
                    currentStep[chan] = (currentStep[chan] + 1) % int(numSteps);
                    break;

                case 3:
                    currentStep[chan] = (currentStep[chan] - 1);
                    if(currentStep[chan] < 0)
                        currentStep[chan] = numSteps - 1;
                    break;

                case 4:
                    currentStep[chan] = (random::u32() % int(numSteps));
                    break;
                
                default:
                    break;
                }
            }

            outputValue = normalizedPhasor;
        }

        //clear outputs
        for (int i = 0; i < NUM_STEPS; i++)
        {
            outputs[PHASOR_OUTPUTS + i].setVoltage(0.0f, chan);
        }
        
        //set output only for current step
        outputs[PHASOR_OUTPUTS + currentStep[chan]].setVoltage(outputValue * HCV_PHZ_UPSCALE, chan);
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

    // Mode Lights
    float modeMod = modeKnob + (modeDepth * inputs[MODECV_INPUT].getVoltage());
    int lightMode = (int)clamp(modeMod, 0.0f, 4.0f);
    for (int i = MODE_LIGHTS; i <= MODE_LIGHTS_LAST; i++)
    {
        lights[i].setBrightness(lightMode == (i - MODE_LIGHTS) ? 5.0f : 0.0f);
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

    //////INPUTS//////
    int jackX = 20;
    int jackX2 = 78;
    int inputY = 320;
    createInputPort(jackX, inputY, PhasorSplitter::PHASOR_INPUT);
    createInputPort(jackX2, inputY, PhasorSplitter::RUN_INPUT);

    createInputPort(46, 270, PhasorSplitter::RESET_INPUT);
    createHCVButtonSmallForJack(46, 270, PhasorSplitter::RESET_PARAM);

    //////OUTPUTS/////
    int outputY = 310;

    for (int i = 0; i < PhasorSplitter::NUM_STEPS; i++)
    {
        int outJackX = 115 + 30 * (i%8);
        int outJackY = 45 + (i/8)*41;

        createOutputPort(outJackX, outJackY, PhasorSplitter::PHASOR_OUTPUTS + i);
        createHCVBipolarLightForJack(outJackX, outJackY, PhasorSplitter::STEP_LIGHTS + 2*i);
    }

    for (int i = PhasorSplitter::MODE_LIGHTS; i <= PhasorSplitter::MODE_LIGHTS_LAST; i++)
    {
        float lightX = 25.0f;
        float lightY = 198.0f + (i - PhasorSplitter::MODE_LIGHTS)*9.75f;
        createHCVRedLight(lightX, lightY, i);
    }   

    createHCVGreenLightForJack(jackX2, inputY, PhasorSplitter::RUN_LIGHT);
}

Model *modelPhasorSplitter = createModel<PhasorSplitter, PhasorSplitterWidget>("PhasorSplitter");
