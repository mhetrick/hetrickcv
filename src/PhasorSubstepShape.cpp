#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorSubstepShape : HCVModule
{
	enum ParamIds
	{
		STEPS_PARAM, STEPS_SCALE_PARAM,
        SHAPE_PARAM, SHAPE_SCALE_PARAM,
        MODE_PARAM, MODE_SCALE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPS_INPUT,
        SHAPE_INPUT,
        MODE_INPUT,
        ACTIVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		SHAPED_OUTPUT,
        PHASORS_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        ENUMS(MODE_LIGHT, 10),
        ACTIVE_LIGHT,
        SHAPED_LIGHT,
        PHASORS_LIGHT,
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;
    static constexpr float MODE_CV_SCALE = 9.0f/5.0f;

	PhasorSubstepShape()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configBypass(PHASOR_INPUT, SHAPED_OUTPUT);

		configParam(SHAPE_PARAM, -5.0, 5.0, 0.0, "Phasor Shape");
		configParam(SHAPE_SCALE_PARAM, -1.0, 1.0, 1.0, "Phasoe Shape CV Depth");

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 0.0, "Steps");
		configParam(STEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;
        
        configSwitch(MODE_PARAM, 0.0, 9.0, 0.0, "Shape Mode", 
        {"Curve", "S-Curve", "Split", "Swing", "Shift", "Triangle", "Arc", "Speed - Clip", "Speed - Wrap", "Speed - Fold"});
        configParam(MODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Shape Mode CV Depth");
        paramQuantities[MODE_PARAM]->snapEnabled = true;

        configInput(PHASOR_INPUT, "Phasor");

        configInput(SHAPE_INPUT, "Phasor Shape CV");
        configInput(STEPS_INPUT, "Steps CV");
        configInput(MODE_INPUT, "Mode CV");
        configInput(ACTIVE_INPUT, "Activation Gate");

        configOutput(SHAPED_OUTPUT, "Shaped Phasor");
        configOutput(PHASORS_OUTPUT, "Shaped Phasor Steps");

        random::init();
	}

    HCVPhasorStepDetector stepDetectors[16];

	void process(const ProcessArgs &args) override;
    float phasorShape(float _phasor, float _parameter, int _mode);

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorSubstepShape::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPS_SCALE_PARAM].getValue() * STEPS_CV_SCALE;

    const float shapeKnob = params[SHAPE_PARAM].getValue();
    const float shapeDepth = params[SHAPE_SCALE_PARAM].getValue();

    const float modeKnob = params[MODE_PARAM].getValue();
    const float modeDepth = params[MODE_SCALE_PARAM].getValue() * MODE_CV_SCALE;

    for (int i = 0; i < numChannels; i++)
    {
        float shape = shapeKnob + (shapeDepth * inputs[SHAPE_INPUT].getPolyVoltage(i));
        shape = clamp(shape, -5.0f, 5.0f) * 0.2f;

        float modeMod = modeKnob + (modeDepth * inputs[MODE_INPUT].getPolyVoltage(i));
        int mode = (int)clamp(modeMod, 0.0f, 9.0f);

        float steps = stepsKnob + (stepsCVDepth * inputs[STEPS_INPUT].getPolyVoltage(i));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));
        stepDetectors[i].setNumberSteps(steps);
        float stepFraction = 1.0f/steps;

        const float phasorInput = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float scaledPhasor = scaleAndWrapPhasor(phasorInput);

        bool active = true;
        if(inputs[ACTIVE_INPUT].isConnected())
        {
            active = inputs[ACTIVE_INPUT].getPolyVoltage(i) >= 1.0f;
        }

        bool stepChanged = stepDetectors[i](scaledPhasor);
        float fractionalPhasor = stepDetectors[i].getFractionalStep();

        float offsetBase = stepFraction * stepDetectors[i].getCurrentStep();

        const float shapedPhasorStep = phasorShape(fractionalPhasor, shape, mode);
        const float phasorOut = offsetBase + shapedPhasorStep * stepFraction;

        outputs[SHAPED_OUTPUT].setVoltage(active ? phasorOut * HCV_PHZ_UPSCALE : phasorInput, i);
        outputs[PHASORS_OUTPUT].setVoltage(shapedPhasorStep * HCV_PHZ_UPSCALE, i);
    }

    float modeMod = modeKnob + (modeDepth * inputs[MODE_INPUT].getVoltage());
    int lightMode = (int)clamp(modeMod, 0.0f, 9.0f);
    for (int i = 0; i < 10; i++)
    {
        lights[i].setBrightness(lightMode == i ? 5.0f : 0.0f);
    }

    bool active = true;
    if(inputs[ACTIVE_INPUT].isConnected())
    {
        active = inputs[ACTIVE_INPUT].getVoltage() >= 1.0f;
    }

    lights[ACTIVE_LIGHT].setBrightness(active ? 1.0f : 0.0f);
    setLightFromOutput(SHAPED_LIGHT, SHAPED_OUTPUT);
    setLightFromOutput(PHASORS_LIGHT, PHASORS_OUTPUT);
}

float PhasorSubstepShape::phasorShape(float _phasor, float _parameter, int _mode)
{
    switch (_mode)
    {
    case 0: return HCVPhasorEffects::phasorCurve(_phasor, _parameter); //curve
    case 1: return HCVPhasorEffects::phasorPinch(_phasor, _parameter); //s-curve
    case 2: return HCVPhasorEffects::phasorKink(_phasor, _parameter); //kink
    case 3: return HCVPhasorEffects::phasorSplit(_phasor, _parameter); //split
    case 4: return HCVPhasorEffects::phasorShift(_phasor, _parameter); //phase shift
    case 5: return HCVPhasorEffects::triangleShaper(_phasor, _parameter); //triangle
    case 6: return HCVPhasorEffects::arcShaper(_phasor, _parameter); //arc
    case 7: return HCVPhasorEffects::speedClip(_phasor, _parameter); //speed clip
    case 8: return HCVPhasorEffects::speedWrap(_phasor, _parameter); //speed wrap
    case 9: return HCVPhasorEffects::speedFold(_phasor, _parameter); //speed fold
    
    default: return _phasor;
    }
}


struct PhasorSubstepShapeWidget : HCVModuleWidget { PhasorSubstepShapeWidget(PhasorSubstepShape *module); };

PhasorSubstepShapeWidget::PhasorSubstepShapeWidget(PhasorSubstepShape *module)
{
	setSkinPath("res/PhasorSubstepShape.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorSubstepShape::STEPS_PARAM, PhasorSubstepShape::STEPS_SCALE_PARAM, PhasorSubstepShape::STEPS_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorSubstepShape::SHAPE_PARAM, PhasorSubstepShape::SHAPE_SCALE_PARAM, PhasorSubstepShape::SHAPE_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorSubstepShape::MODE_PARAM, PhasorSubstepShape::MODE_SCALE_PARAM, PhasorSubstepShape::MODE_INPUT);

    const float jackY = 316.0f;
    const float activeX = 45.0f;
	//////INPUTS//////
    createInputPort(9, jackY, PhasorSubstepShape::PHASOR_INPUT);
    createInputPort(activeX, jackY, PhasorSubstepShape::ACTIVE_INPUT);

	//////OUTPUTS//////
    createOutputPort(94.0f, jackY, PhasorSubstepShape::SHAPED_OUTPUT);
    createOutputPort(136.0f, jackY, PhasorSubstepShape::PHASORS_OUTPUT);
    

    createHCVRedLightForJack(activeX, jackY, PhasorSubstepShape::ACTIVE_LIGHT);
    createHCVRedLightForJack(94.0f, jackY, PhasorSubstepShape::SHAPED_LIGHT);
    createHCVRedLightForJack(136.0f, jackY, PhasorSubstepShape::PHASORS_LIGHT);

    int halfLights = 5;
    int lightX = 82;
    for (int i = 0; i < halfLights; i++)
    {
        float lightY = 237 + i*10.0f;
        createHCVRedLight(lightX, lightY, i);
        createHCVRedLight(lightX + 11, lightY, i + halfLights);
    }   
}

Model *modelPhasorSubstepShape = createModel<PhasorSubstepShape, PhasorSubstepShapeWidget>("PhasorSubstepShape");
