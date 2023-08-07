#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorToClock : HCVModule
{
	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        WIDTH_PARAM,
        WIDTHCV_PARAM,
        DETECTION_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        WIDTHCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        CLOCK_OUTPUT,
        PHASOR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        CLOCK_LIGHT,
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

    HCVPhasorStepDetector stepDetectors[16];
    HCVPhasorGateDetector gateDetectors[16];

	PhasorToClock()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 16.0, "Steps");
		configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(WIDTH_PARAM, -5.0, 5.0, 0.0, "Gate Width");
        configParam(WIDTHCV_PARAM, -1.0, 1.0, 1.0, "Gate Width CV Depth");

        configSwitch(DETECTION_PARAM, 0.0, 1.0, 1.0, "Detection Mode", {"Raw", "Smart (Detect Playback and Reverse)"});

        configInput(PHASOR_INPUT, "Phasor");
        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(WIDTHCV_INPUT, "Width CV");

        configOutput(PHASOR_OUTPUT, "Clock Subphasors");
        configOutput(CLOCK_OUTPUT, "Clock Gates");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {

    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorToClock::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float widthKnob = params[WIDTH_PARAM].getValue();
    const float widthDepth = params[WIDTHCV_PARAM].getValue();

    const bool smartDetection = params[DETECTION_PARAM].getValue() > 0.0f;

    for (int i = 0; i < numChannels; i++)
    {
        float steps = stepsKnob + (stepsCVDepth * inputs[STEPSCV_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));
        stepDetectors[i].setNumberSteps(steps);
        float stepFraction = 1.0f/steps;

        float pulseWidth = widthKnob + (widthDepth * inputs[WIDTHCV_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;

        gateDetectors[i].setSmartMode(smartDetection);
        gateDetectors[i].setGateWidth(pulseWidth);

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        bool stepAdvanced = stepDetectors[i](normalizedPhasor);

        const float fractionalStep = stepDetectors[i].getFractionalStep();
        float gate = gateDetectors[i](fractionalStep);

        outputs[PHASOR_OUTPUT].setVoltage(fractionalStep * HCV_PHZ_UPSCALE, i);
        outputs[CLOCK_OUTPUT].setVoltage(gate, i);
    }

    lights[CLOCK_LIGHT].setBrightness(outputs[CLOCK_OUTPUT].getVoltage());
    
}

struct PhasorToClockWidget : HCVModuleWidget { PhasorToClockWidget(PhasorToClock *module); };

PhasorToClockWidget::PhasorToClockWidget(PhasorToClock *module)
{
    setSkinPath("res/PhasorToClock.svg");
    initializeWidget(module);
    
    int knobY = 90;

    //////PARAMS//////
    createParamComboVertical(15, knobY, PhasorToClock::STEPS_PARAM, PhasorToClock::STEPSCV_PARAM, PhasorToClock::STEPSCV_INPUT);
    createParamComboVertical(70, knobY, PhasorToClock::WIDTH_PARAM, PhasorToClock::WIDTHCV_PARAM, PhasorToClock::WIDTHCV_INPUT);

    createHCVSwitchVert(89, 252, PhasorToClock::DETECTION_PARAM);

    //////INPUTS//////
    int leftX = 21;
    int rightX = 76;
    int topJackY = 245;
    int bottomJackY = 310;
    createInputPort(leftX, topJackY, PhasorToClock::PHASOR_INPUT);
    
    createOutputPort(leftX, bottomJackY, PhasorToClock::CLOCK_OUTPUT);
    createOutputPort(rightX, bottomJackY, PhasorToClock::PHASOR_OUTPUT);

    createHCVRedLight(leftX - 5, bottomJackY - 2, PhasorToClock::CLOCK_LIGHT);
    
}

Model *modelPhasorToClock = createModel<PhasorToClock, PhasorToClockWidget>("PhasorToClock");
