#include "HetrickCV.hpp"
#include "DSP/HCVPhasorEffects.h"

struct PhasorShape : HCVModule
{
	enum ParamIds
	{
        SHAPE_PARAM,
        SHAPECV_PARAM,
        MODE_PARAM,
        MODECV_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        SHAPECV_INPUT,
        MODECV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS = 5
	};

	PhasorShape()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(SHAPE_PARAM, -5.0, 5.0, 0.0, "Phasor Shape");
        configParam(SHAPECV_PARAM, -1.0, 1.0, 1.0, "Phasor Shape CV Depth");

        configSwitch(MODE_PARAM, 0.0, 4.0, 0.0, "Shape Mode", 
        {"Curve", "S-Curve", "Kink", "Split", "Mirror"});
        configParam(MODECV_PARAM, -1.0, 1.0, 1.0, "Shape Mode CV Depth");

        configInput(PHASOR_INPUT, "Phasor");
        configInput(SHAPECV_INPUT, "Phasor Shape CV");
        configInput(MODECV_INPUT, "Shape Mode CV");

        configOutput(PHASOR_OUTPUT, "Shaped Phasor");

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


void PhasorShape::process(const ProcessArgs &args)
{
    const float shapeKnob = params[SHAPE_PARAM].getValue();
    const float shapeDepth = params[SHAPECV_PARAM].getValue();

    const float modeKnob = params[MODE_PARAM].getValue();
    const float modeDepth = params[MODECV_PARAM].getValue();

    int numChannels = getMaxInputPolyphony();
    outputs[PHASOR_OUTPUT].setChannels(numChannels);

    for (int i = 0; i < numChannels; i++)
    {
        float shape = shapeKnob + (shapeDepth * inputs[SHAPECV_INPUT].getPolyVoltage(i));
        shape = clamp(shape, -5.0f, 5.0f) * 0.2f;

        float phasorInput = inputs[PHASOR_INPUT].getPolyVoltage(i);
        float scaledPhasor = scaleAndWrapPhasor(phasorInput);

        float shapedOutput = HCVPhasorEffects::phasorCurve(scaledPhasor, shape);

        outputs[PHASOR_OUTPUT].setVoltage(shapedOutput * 5.0f, i);
    }
}

struct PhasorShapeWidget : HCVModuleWidget { PhasorShapeWidget(PhasorShape *module); };

PhasorShapeWidget::PhasorShapeWidget(PhasorShape *module)
{
    setSkinPath("res/PhasorShape.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    int jackY = 310;
    createInputPort(21, jackY, PhasorShape::PHASOR_INPUT);
    int knobY = 90;
    createParamComboVertical(15, knobY, PhasorShape::SHAPE_PARAM, PhasorShape::SHAPECV_PARAM, PhasorShape::SHAPECV_INPUT);
    createParamComboVertical(70, knobY, PhasorShape::MODE_PARAM, PhasorShape::MODECV_PARAM, PhasorShape::MODECV_INPUT);
    

    //createHCVRedLight(75, 320, PhasorShape::DELAY_LIGHT);
    createOutputPort(70, jackY, PhasorShape::PHASOR_OUTPUT);
    
}

Model *modelPhasorShape = createModel<PhasorShape, PhasorShapeWidget>("PhasorShape");
