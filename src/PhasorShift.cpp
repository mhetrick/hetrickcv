#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorShift : HCVModule
{
	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        SHIFT_PARAM,
        SHIFTCV_PARAM,
        QUANTIZE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        SHIFTCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_OUTPUT,
        GATES_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        QUANTIZE_LIGHT,
        PHASOR_LIGHT,
        GATES_LIGHT,
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

	PhasorShift()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, PHASOR_OUTPUT);

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 0.0, "Steps");
		configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(SHIFT_PARAM, -5.0, 5.0, 0.0, "Phasor Shift");
        configParam(SHIFTCV_PARAM, -1.0, 1.0, 1.0, "Phasor Shift CV Depth");

        configInput(PHASOR_INPUT, "Phasor");
        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(SHIFTCV_INPUT, "Shift CV");

        configOutput(PHASOR_OUTPUT, "Shifted Phasor");
        configOutput(GATES_OUTPUT, "Shifted Step Gates");

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


void PhasorShift::process(const ProcessArgs &args)
{
    const int numChannels = setupPolyphonyForAllOutputs();

    const float stepsKnob = params[STEPS_PARAM].getValue();
    const float stepsCVDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float shiftKnob = params[SHIFT_PARAM].getValue();
    const float shiftDepth = params[SHIFTCV_PARAM].getValue();

    bool quantize = params[QUANTIZE_PARAM].getValue() > 0.0f;

    for (int i = 0; i < numChannels; i++)
    {
        float steps = stepsKnob + (stepsCVDepth * inputs[STEPSCV_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));
        float stepFraction = 1.0f/steps;

        float shift = shiftKnob + (shiftDepth * inputs[SHIFTCV_INPUT].getPolyVoltage(i));
        shift = clamp(shift, -5.0f, 5.0f) * 0.2f;

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(i));

        if(quantize)
        {
            shift = HCVPhasorDivMult::roundTruncMultiple(shift, stepFraction);
            shift = clamp(shift, -1.0f, 1.0f);
        }

        float shiftedPhasor = gam::scl::wrap(normalizedPhasor + shift);
        bool gate = shiftedPhasor < 0.5f;

        outputs[PHASOR_OUTPUT].setVoltage(shiftedPhasor * HCV_PHZ_UPSCALE, i);
        outputs[GATES_OUTPUT].setVoltage(gate ? HCV_PHZ_GATESCALE : 0.0f, i);
    }

    lights[QUANTIZE_LIGHT].setBrightness(quantize ? 1.0f : 0.0f);
    setLightFromOutput(PHASOR_LIGHT, PHASOR_OUTPUT);
    setLightFromOutput(GATES_LIGHT, GATES_OUTPUT);
}

struct PhasorShiftWidget : HCVModuleWidget { PhasorShiftWidget(PhasorShift *module); };

PhasorShiftWidget::PhasorShiftWidget(PhasorShift *module)
{
    setSkinPath("res/PhasorShift.svg");
    initializeWidget(module);
    
    int knobY = 90;

    //////PARAMS//////
    createParamComboVertical(15, knobY, PhasorShift::STEPS_PARAM, PhasorShift::STEPSCV_PARAM, PhasorShift::STEPSCV_INPUT);
    createParamComboVertical(70, knobY, PhasorShift::SHIFT_PARAM, PhasorShift::SHIFTCV_PARAM, PhasorShift::SHIFTCV_INPUT);


    //////INPUTS//////
    int leftX = 21;
    int rightX = 76;
    int topJackY = 245;
    int bottomJackY = 310;
    createInputPort(leftX, topJackY, PhasorShift::PHASOR_INPUT);
    createHCVSwitchHoriz(rightX, topJackY + 5, PhasorShift::QUANTIZE_PARAM);
    
    createOutputPort(leftX, bottomJackY, PhasorShift::PHASOR_OUTPUT);
    createOutputPort(rightX, bottomJackY, PhasorShift::GATES_OUTPUT);

    createHCVRedLight(rightX + 30, topJackY, PhasorShift::QUANTIZE_LIGHT);

    createHCVRedLightForJack(leftX, bottomJackY, PhasorShift::PHASOR_LIGHT);
    createHCVRedLightForJack(rightX, bottomJackY, PhasorShift::GATES_LIGHT);
}

Model *modelPhasorShift = createModel<PhasorShift, PhasorShiftWidget>("PhasorShift");
