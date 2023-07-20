#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorToLFO : HCVModule
{

	enum ParamIds
	{
		SKEW_PARAM, SKEW_SCALE_PARAM,
        WIDTH_PARAM, WIDTH_SCALE_PARAM,
        SHAPE_PARAM, SHAPE_SCALE_PARAM,
        CURVE_PARAM, CURVE_SCALE_PARAM,

        BIPOLAR_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,

        SKEW_INPUT,
        WIDTH_INPUT,
        SHAPE_INPUT,
        CURVE_INPUT,

		NUM_INPUTS
	};
	enum OutputIds
	{
		MAIN_OUTPUT,
        TRI_OUTPUT,
        PULSE_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        LFO_POS_LIGHT, LFO_NEG_LIGHT,
        TRI_POS_LIGHT, TRI_NEG_LIGHT,
        PULSE_POS_LIGHT, PULSE_NEG_LIGHT,
        NUM_LIGHTS
	};

	PhasorToLFO()
	{

		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SKEW_PARAM, -5.0f, 5.0f, 0.0f, "Skew");
		configParam(SKEW_SCALE_PARAM, -1.0, 1.0, 1.0, "Skew CV Depth");

        configParam(WIDTH_PARAM, -5.0f, 5.0f, 0.0, "Width");
		configParam(WIDTH_SCALE_PARAM, -1.0, 1.0, 1.0, "Width CV Depth");

        configParam(SHAPE_PARAM, -5.0f, 5.0f, 0.0, "Shape");
		configParam(SHAPE_SCALE_PARAM, -1.0, 1.0, 1.0, "Shape CV Depth");

        configParam(CURVE_PARAM, -5.0f, 5.0f, 0.0, "Curve");
		configParam(CURVE_SCALE_PARAM, -1.0, 1.0, 1.0, "Curve CV Depth");

        configSwitch(BIPOLAR_PARAM, 0.0f, 1.0f, 0.0f, "Polarity", {"Unipolar", "Bipolar"});

        configInput(PHASOR_INPUT, "Phasor");

        configInput(SKEW_INPUT, "Skew CV");
        configInput(WIDTH_INPUT, "Width CV");
        configInput(SHAPE_INPUT, "Shape CV");
        configInput(CURVE_INPUT, "Curve CV");

        configOutput(MAIN_OUTPUT, "Main LFO");
        configOutput(TRI_OUTPUT, "Triangle LFO");
        configOutput(PULSE_OUTPUT, "Pulse LFO");
	}

	void process(const ProcessArgs &args) override;

    HCVPhasorLFO lfos[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorToLFO::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float skewKnob = params[SKEW_PARAM].getValue();
    float widthKnob = params[WIDTH_PARAM].getValue();
    float shapeKnob = params[SHAPE_PARAM].getValue();
    float curveKnob = params[CURVE_PARAM].getValue();

    float skewCVDepth = params[SKEW_SCALE_PARAM].getValue();
    float widthCVDepth = params[WIDTH_SCALE_PARAM].getValue();
    float shapeCVDepth = params[SHAPE_SCALE_PARAM].getValue();
    float curveCVDepth = params[CURVE_SCALE_PARAM].getValue();
    float offset = -5.0f * params[BIPOLAR_PARAM].getValue();

    for (int i = 0; i < numChannels; i++)
    {
        float skew = skewKnob + (skewCVDepth * inputs[SKEW_INPUT].getPolyVoltage(i));
        skew = clamp(skew, -5.0f, 5.0f) * 0.2;
        lfos[i].setPhaseParam(skew);

        float width = widthKnob + (widthCVDepth * inputs[WIDTH_INPUT].getPolyVoltage(i));
        width = clamp(width, -5.0f, 5.0f) * 0.1f + 0.5f;
        lfos[i].setWidthParam(width);

        float shape = shapeKnob + (shapeCVDepth * inputs[SHAPE_INPUT].getPolyVoltage(i));
        shape = clamp(shape, -5.0f, 5.0f) * 0.1f + 0.5f;
        lfos[i].setTrapezoidParam(shape);

        float curve = curveKnob + (curveCVDepth * inputs[CURVE_INPUT].getPolyVoltage(i));
        curve = clamp(curve, -5.0f, 5.0f) * 0.1f + 0.5f;
        lfos[i].setSinusoidParam(curve);

        const float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getVoltage(i));
        const float mainLFO = lfos[i](normalizedPhasor);

        outputs[MAIN_OUTPUT].setVoltage(mainLFO * HCV_PHZ_UPSCALE + offset, i);
        outputs[TRI_OUTPUT].setVoltage(lfos[i].getTriangle() * HCV_PHZ_UPSCALE + offset, i);
        outputs[PULSE_OUTPUT].setVoltage(lfos[i].getPulse() + offset, i);
    }

    setBipolarLightBrightness(LFO_POS_LIGHT, outputs[MAIN_OUTPUT].getVoltage() * 0.2f);
    setBipolarLightBrightness(TRI_POS_LIGHT, outputs[TRI_OUTPUT].getVoltage() * 0.2f);
    setBipolarLightBrightness(PULSE_POS_LIGHT, outputs[PULSE_OUTPUT].getVoltage() * 0.2f);
}


struct PhasorToLFOWidget : HCVModuleWidget { PhasorToLFOWidget(PhasorToLFO *module); };

PhasorToLFOWidget::PhasorToLFOWidget(PhasorToLFO *module)
{
	setSkinPath("res/PhasorToLFO.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 64.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorToLFO::SKEW_PARAM, PhasorToLFO::SKEW_SCALE_PARAM, PhasorToLFO::SKEW_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorToLFO::WIDTH_PARAM, PhasorToLFO::WIDTH_SCALE_PARAM, PhasorToLFO::WIDTH_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorToLFO::SHAPE_PARAM, PhasorToLFO::SHAPE_SCALE_PARAM, PhasorToLFO::SHAPE_INPUT);
    createParamComboHorizontal(knobX, knobY + 150, PhasorToLFO::CURVE_PARAM, PhasorToLFO::CURVE_SCALE_PARAM, PhasorToLFO::CURVE_INPUT);


    const float jackY = 305.0f;
	//////INPUTS//////
    createInputPort(11.0f, jackY, PhasorToLFO::PHASOR_INPUT);

	//////OUTPUTS//////
    createOutputPort(60.0f, jackY, PhasorToLFO::MAIN_OUTPUT);
    createOutputPort(100.0f, jackY, PhasorToLFO::TRI_OUTPUT);
    createOutputPort(140.0f, jackY, PhasorToLFO::PULSE_OUTPUT);

    createHCVSwitchHoriz(80.0f, 263.0f, PhasorToLFO::BIPOLAR_PARAM);

    const float lightY = 300.0f;
    createHCVBipolarLight(85.0, lightY, PhasorToLFO::LFO_POS_LIGHT);
    createHCVBipolarLight(125.0, lightY, PhasorToLFO::TRI_POS_LIGHT);
    createHCVBipolarLight(165.0, lightY, PhasorToLFO::PULSE_POS_LIGHT);
    
}

Model *modelPhasorToLFO = createModel<PhasorToLFO, PhasorToLFOWidget>("PhasorToLFO");
