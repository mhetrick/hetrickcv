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

        FILLMODE_PARAM, FILLMODE_SCALE_PARAM,

        STEPSQUANTIZE_PARAM,
        FILLQUANTIZE_PARAM,
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
        FILLMODE_INPUT,

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
		configParam(BEATS_PARAM, 1.0f, MAX_BEATS, 16.0f, "Beats");
		configParam(BEATS_SCALE_PARAM, -1.0, 1.0, 1.0, "Beats CV Depth");

        configParam(FILL_PARAM, 0.0f, MAX_BEATS, 4.0, "Fill");
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
        paramQuantities[FILLMODE_PARAM]->snapEnabled = true;
        configParam(FILLMODE_SCALE_PARAM, -1.0, 1.0, 1.0, "Fill Limit Mode CV Depth");

        configSwitch(STEPSQUANTIZE_PARAM, 0.0, 1.0, 1.0, "Quantize Steps Parameter", 
        {"Free, Non-Integer Step Values", "Quantized, Whole Number Step Values"});
        configSwitch(FILLQUANTIZE_PARAM, 0.0, 1.0, 1.0, "Quantize Fill Parameter", 
        {"Free, Non-Integer Fill Values", "Quantized, Whole Number Fill Values"});


        configSwitch(QUANTIZE_PARAM, 0.0, 1.0, 1.0, "Quantize Param Changes", 
        {"Instant Parameter Changes", "Quantize Param Changes to Step Changes"});
        configSwitch(DETECTION_PARAM, 0.0, 1.0, 1.0, "Detection Mode", {"Raw", "Smart (Detect Playback and Reverse)"});
        
        

        configInput(PHASOR_INPUT, "Phasor");

        configInput(BEATS_INPUT, "Beats CV");
        configInput(FILL_INPUT, "Fill CV");
        configInput(ROTATE_INPUT, "Rotate CV");
        configInput(PW_INPUT, "Pulse Width CV");
        configInput(FILLMODE_INPUT, "Fill Mode CV");

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

    float fillModeKnob = params[FILLMODE_PARAM].getValue();
    const float fillModeCVDepth = params[FILLMODE_SCALE_PARAM].getValue();

    paramQuantities[BEATS_PARAM]->snapEnabled = params[STEPSQUANTIZE_PARAM].getValue() > 0.0f;
    
    bool fillQuantized = params[FILLQUANTIZE_PARAM].getValue() > 0.0f;
    paramQuantities[FILL_PARAM]->snapEnabled = fillModeKnob == 5.0f ? false : fillQuantized;


    for (int i = 0; i < numChannels; i++)
    {
        euclidean[i].setParameterChangeQuantization(quantizeParamChanges);
        euclidean[i].enableSmartDetection(smartDetection);

        float beats = beatKnob + (beatCVDepth * inputs[BEATS_INPUT].getPolyVoltage(i));
        beats = clamp(beats, 1.0f, MAX_BEATS);
        float fill = fillKnob + (fillCVDepth * inputs[FILL_INPUT].getPolyVoltage(i));
        fill = clamp(fill, 0.0f, MAX_BEATS);

        float fillMode = fillModeKnob + (fillCVDepth * inputs[FILLMODE_INPUT].getPolyVoltage(i));
        fillMode = clamp(fillMode, 0.0f, 5.0f);

        switch ((int) fillMode)
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
            if(fillQuantized) euclidean[i].setFill(std::round(beats * fill));
            else euclidean[i].setFill(beats * fill);
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
        lights[FILLMODE_LIGHTS + i].setBrightness((int) fillModeKnob == i ? 1.0f : 0.0f);
    }
}


struct PhasorEuclideanWidget : HCVModuleWidget { PhasorEuclideanWidget(PhasorEuclidean *module); };

PhasorEuclideanWidget::PhasorEuclideanWidget(PhasorEuclidean *module)
{
	setSkinPath("res/PhasorEuclidean.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 39.0f;
    const float knobX = 65.0f;

    const float stepsX = 15.0f;
    const float stepsY = 60.0f;
    createParamComboVertical(stepsX, stepsY, PhasorEuclidean::BEATS_PARAM, PhasorEuclidean::BEATS_SCALE_PARAM, PhasorEuclidean::BEATS_INPUT);

    
    createParamComboHorizontal(knobX, knobY, PhasorEuclidean::FILL_PARAM, PhasorEuclidean::FILL_SCALE_PARAM, PhasorEuclidean::FILL_INPUT);
    createParamComboHorizontal(knobX, knobY + 50, PhasorEuclidean::ROTATE_PARAM, PhasorEuclidean::ROTATE_SCALE_PARAM, PhasorEuclidean::ROTATE_INPUT);
    createParamComboHorizontal(knobX, knobY + 100, PhasorEuclidean::PW_PARAM, PhasorEuclidean::PW_SCALE_PARAM, PhasorEuclidean::PW_INPUT);
    createParamComboHorizontal(knobX, knobY + 150, PhasorEuclidean::FILLMODE_PARAM, PhasorEuclidean::FILLMODE_SCALE_PARAM, PhasorEuclidean::FILLMODE_INPUT);

    const float jackY = 318.0f;
    float xSpacing = 41.0f;


    float jackX2 = 93.0;
    float jackX3 = jackX2 + xSpacing;
    float jackX4 = jackX3 + xSpacing;

    const float switchY = 275;

    createHCVSwitchVert(12, switchY, PhasorEuclidean::STEPSQUANTIZE_PARAM);
    createHCVSwitchVert(52, switchY, PhasorEuclidean::FILLQUANTIZE_PARAM);
    createHCVSwitchVert(92, switchY, PhasorEuclidean::QUANTIZE_PARAM);
    createHCVSwitchVert(132, switchY, PhasorEuclidean::DETECTION_PARAM);
    
	//////INPUTS//////
    createInputPort(43.0f, jackY, PhasorEuclidean::PHASOR_INPUT);

	//////OUTPUTS//////
    createOutputPort(jackX2, jackY, PhasorEuclidean::PHASOR_OUTPUT);
    createOutputPort(jackX3, jackY, PhasorEuclidean::GATE_OUTPUT);
    createOutputPort(jackX4, jackY, PhasorEuclidean::CLOCK_OUTPUT);

    createHCVRedLightForJack(jackX2, jackY, PhasorEuclidean::PHASOR_LIGHT);
    createHCVRedLightForJack(jackX3, jackY, PhasorEuclidean::GATE_LIGHT);
    createHCVRedLightForJack(jackX4, jackY, PhasorEuclidean::CLOCK_LIGHT);

    for (int i = 0; i < 6; i++)
    {
        createHCVRedLight(160.0f, 242.0f + i*9.5f, PhasorEuclidean::FILLMODE_LIGHTS + i);
    }
    
}

Model *modelPhasorEuclidean = createModel<PhasorEuclidean, PhasorEuclideanWidget>("PhasorEuclidean");
