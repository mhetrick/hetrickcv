#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorEuclidean : HCVModule
{
    static constexpr float MAX_BEATS = 64.0f;
    static constexpr float BEATS_CV_SCALE = MAX_BEATS/5.0f;

	enum ParamIds
	{
		BEATS_PARAM, BEATS_SCALE_PARAM,
        FILL_PARAM, FILL_SCALE_PARAM,
        ROTATE_PARAM, ROTATE_SCALE_PARAM,
        PW_PARAM, PW_SCALE_PARAM,

        FILLMODE_PARAM,
        QUANTIZE_PARAM,
        DETECTION_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,

        BEATS_INPUT,
        FILL_INPUT,
        ROTATE_INPUT,
        PW_INPUT,

		NUM_INPUTS
	};
	enum OutputIds
	{
		PHASOR_OUTPUT,
        GATE_OUTPUT,
        CLOCK_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        PHASOR_LIGHT,
        GATE_LIGHT,
        CLOCK_LIGHT,

        ENUMS(FILLMODE_LIGHTS, 6),
        NUM_LIGHTS
	};

	PhasorEuclidean()
	{

		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(BEATS_PARAM, 1.0f, MAX_BEATS, 1.0f, "Beats");
		configParam(BEATS_SCALE_PARAM, -1.0, 1.0, 1.0, "Beats CV Depth");

        configParam(FILL_PARAM, 0.0f, MAX_BEATS, 0.0, "Fill");
		configParam(FILL_SCALE_PARAM, -1.0, 1.0, 1.0, "Fill CV Depth");

        configParam(ROTATE_PARAM, -5.0f, 5.0f, 0.0, "Rotate");
		configParam(ROTATE_SCALE_PARAM, -1.0, 1.0, 1.0, "Rotate CV Depth");

        configParam(PW_PARAM, -5.0f, 5.0f, 0.0, "Pulse Width");
		configParam(PW_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulse Width CV Depth");

        paramQuantities[BEATS_PARAM]->snapEnabled = true;
        paramQuantities[FILL_PARAM]->snapEnabled = true;

        configSwitch(FILLMODE_PARAM, 0.0, 5.0, 0.0, "Fill Parameter Limiting Mode", 
        {"Limit Fill to Current Number of Steps", "Swap Fill and Steps Depending on Which is Larger",
        "Wrap Fill Between 0 and Steps", "Fold Fill Between 0 and Steps", "Euclidean Ratchet Mode", "Density Mode"});
        configSwitch(QUANTIZE_PARAM, 0.0, 1.0, 1.0, "Quantize Param Changes", 
        {"Instant Parameter Changes", "Quantize Param Changes to Step Changes"});
        configSwitch(DETECTION_PARAM, 0.0, 1.0, 1.0, "Detection Mode", {"Raw", "Smart (Detect Playback and Reverse)"});
        
        paramQuantities[FILLMODE_PARAM]->snapEnabled = true;

        configInput(PHASOR_INPUT, "Phasor");

        configInput(BEATS_INPUT, "Beats CV");
        configInput(FILL_INPUT, "Fill CV");
        configInput(ROTATE_INPUT, "Rotate CV");
        configInput(PW_INPUT, "Pulse Width CV");

        configOutput(PHASOR_OUTPUT, "Euclidean Phasors");
        configOutput(GATE_OUTPUT, "Euclidean Gates");
        configOutput(CLOCK_OUTPUT, "Beats Clock");
	}

	void process(const ProcessArgs &args) override;

    HCVPhasorToEuclidean euclidean[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorEuclidean::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float beatKnob = params[BEATS_PARAM].getValue();
    float fillKnob = params[FILL_PARAM].getValue();
    float rotateKnob = params[ROTATE_PARAM].getValue();
    float pwKnob = params[PW_PARAM].getValue();

    float beatCVDepth = params[BEATS_SCALE_PARAM].getValue() * BEATS_CV_SCALE;
    float fillCVDepth = params[FILL_SCALE_PARAM].getValue() * BEATS_CV_SCALE;
    float rotateCVDepth = params[ROTATE_SCALE_PARAM].getValue();
    float pwCVDepth = params[PW_SCALE_PARAM].getValue();

    const bool quantizeParamChanges = params[QUANTIZE_PARAM].getValue() > 0.0f;
    const bool smartDetection = params[DETECTION_PARAM].getValue() > 0.0f;

    const int fillMode = params[FILLMODE_PARAM].getValue();

    paramQuantities[FILL_PARAM]->snapEnabled = fillMode != 5;

    for (int i = 0; i < numChannels; i++)
    {
        euclidean[i].setParameterChangeQuantization(quantizeParamChanges);
        euclidean[i].enableSmartDetection(smartDetection);

        float beats = beatKnob + (beatCVDepth * inputs[BEATS_INPUT].getPolyVoltage(i));
        beats = clamp(beats, 1.0f, MAX_BEATS);
        float fill = fillKnob + (fillCVDepth * inputs[FILL_INPUT].getPolyVoltage(i));
        fill = clamp(fill, 0.0f, MAX_BEATS);

        switch (fillMode)
        {
        case 0: //clamp
            fill = clamp(fill, 0.0f, beats);
            euclidean[i].setBeats(beats);
            euclidean[i].setFill(fill);
            break;

        case 1: //swap
            if(beats > fill)
            {
                euclidean[i].setBeats(beats);
                euclidean[i].setFill(fill);
            }
            else
            {
                euclidean[i].setBeats(fill);
                euclidean[i].setFill(beats);
            }
            break;

        case 2: //wrap
            fill = gam::scl::wrap(fill, beats, 0.0f);
            euclidean[i].setBeats(beats);
            euclidean[i].setFill(fill);
            break;

        case 3: //fold
            fill = gam::scl::fold(fill, beats, 0.0f);
            euclidean[i].setBeats(beats);
            euclidean[i].setFill(fill);
            break;

        case 4: //ratchet (no clamping, logic handled in effect)
            euclidean[i].setBeats(beats);
            euclidean[i].setFill(fill);
            break;

        case 5: //density mode
            fill = fill/MAX_BEATS; //scale to [0.0, 1.0]
            euclidean[i].setBeats(beats);
            euclidean[i].setFill(beats * fill);
            break;

        default:
            euclidean[i].setBeats(beats);
            euclidean[i].setFill(fill);
            break;
        }

        

        float pulseWidth = pwKnob + (pwCVDepth * inputs[PW_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;
        euclidean[i].setPulseWidth(pulseWidth);

        float rotation = rotateKnob + (rotateCVDepth * inputs[ROTATE_INPUT].getPolyVoltage(i));
        rotation = clamp(rotation, -5.0f, 5.0f) * 0.2f;
        euclidean[i].setRotation(rotation);

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getVoltage(i));
        euclidean[i].processPhasor(normalizedPhasor);

        outputs[PHASOR_OUTPUT].setVoltage(euclidean[i].getPhasorOutput(), i);
        outputs[GATE_OUTPUT].setVoltage(euclidean[i].getEuclideanGateOutput(), i);
        outputs[CLOCK_OUTPUT].setVoltage(euclidean[i].getClockOutput(), i);
    }

    lights[PHASOR_LIGHT].setBrightness(outputs[PHASOR_OUTPUT].getVoltage() * 0.1f);
    lights[GATE_LIGHT].setBrightness(outputs[GATE_OUTPUT].getVoltage() * 0.1f);
    lights[CLOCK_LIGHT].setBrightness(outputs[CLOCK_OUTPUT].getVoltage() * 0.1f);

    for(int i = 0; i < 6; i++)
    {
        lights[FILLMODE_LIGHTS + i].setBrightness(fillMode == i ? 1.0f : 0.0f);
    }
}


struct PhasorEuclideanWidget : HCVModuleWidget { PhasorEuclideanWidget(PhasorEuclidean *module); };

PhasorEuclideanWidget::PhasorEuclideanWidget(PhasorEuclidean *module)
{
	setSkinPath("res/PhasorEuclidean.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 39.0f;
    const float knobX = 10.0f;

    createParamComboHorizontal(knobX, knobY, PhasorEuclidean::BEATS_PARAM, PhasorEuclidean::BEATS_SCALE_PARAM, PhasorEuclidean::BEATS_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorEuclidean::FILL_PARAM, PhasorEuclidean::FILL_SCALE_PARAM, PhasorEuclidean::FILL_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorEuclidean::ROTATE_PARAM, PhasorEuclidean::ROTATE_SCALE_PARAM, PhasorEuclidean::ROTATE_INPUT);
    createParamComboHorizontal(knobX, knobY + 150, PhasorEuclidean::PW_PARAM, PhasorEuclidean::PW_SCALE_PARAM, PhasorEuclidean::PW_INPUT);


    const float jackY = 318.0f;
    float xSpacing = 41.0f;

    float jackX2 = 63.0;
    float jackX3 = jackX2 + xSpacing;
    float jackX4 = jackX3 + xSpacing;

    createHCVTrimpot(70.0f, 240.0f, PhasorEuclidean::FILLMODE_PARAM);

    createHCVSwitchVert(jackX3 + 5, 275, PhasorEuclidean::QUANTIZE_PARAM);
    createHCVSwitchVert(jackX4 + 5, 275, PhasorEuclidean::DETECTION_PARAM);
    
	//////INPUTS//////
    createInputPort(13.0f, jackY, PhasorEuclidean::PHASOR_INPUT);

	//////OUTPUTS//////
    createOutputPort(jackX2, jackY, PhasorEuclidean::PHASOR_OUTPUT);
    createOutputPort(jackX3, jackY, PhasorEuclidean::GATE_OUTPUT);
    createOutputPort(jackX4, jackY, PhasorEuclidean::CLOCK_OUTPUT);

    createHCVRedLightForJack(jackX2, jackY, PhasorEuclidean::PHASOR_LIGHT);
    createHCVRedLightForJack(jackX3, jackY, PhasorEuclidean::GATE_LIGHT);
    createHCVRedLightForJack(jackX4, jackY, PhasorEuclidean::CLOCK_LIGHT);

    for (int i = 0; i < 6; i++)
    {
        createHCVRedLight(15.0f, 252.0f + i*9.5f, PhasorEuclidean::FILLMODE_LIGHTS + i);
    }
    
}

Model *modelPhasorEuclidean = createModel<PhasorEuclidean, PhasorEuclideanWidget>("PhasorEuclidean");
