#include "HetrickCV.hpp"
#include "DSP/HCVRandom.h"
#include "DSP/HCVTiming.h"
#include "DSP/Phasors/HCVPhasorAnalyzers.h"

struct PhasorProbability : HCVModule
{
	enum ParamIds
	{
        PROB_PARAM,
        PROBCV_PARAM,
        PROBBUTTON_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        PROBCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUTA_PHASE_OUTPUT,
        OUTA_GATE_OUTPUT,
        OUTB_PHASE_OUTPUT,
        OUTB_GATE_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        OUTA_PHASE_LIGHT,
        OUTA_GATE_LIGHT,
        OUTB_PHASE_LIGHT,
        OUTB_GATE_LIGHT,

        PROBMODE1_LIGHT,
        PROBMODE2_LIGHT,
        PROBMODE3_LIGHT,

        NUM_LIGHTS
	};

    dsp::SchmittTrigger probButtonTrigger, outButtonTrigger;
    HCVPhasorResetDetector resetDetectors[16];
    bool outALogic[16];
    bool outBLogic[16];
    int probMode = 0;

    HCVRandom randomGen;

	PhasorProbability()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(PhasorProbability::PROB_PARAM, 0.0, 1.0, 0.5, "PhasorProbability");
        configParam(PhasorProbability::PROBCV_PARAM, -1.0, 1.0, 1.0, "PhasorProbability CV Depth");

        configButton(PhasorProbability::PROBBUTTON_PARAM, "PhasorProbability Mode");

        configInput(PhasorProbability::PHASOR_INPUT, "Phasor");
        configInput(PhasorProbability::PROBCV_INPUT, "PhasorProbability CV");

        configOutput(PhasorProbability::OUTA_PHASE_OUTPUT, "A Phase");
        configOutput(PhasorProbability::OUTA_GATE_OUTPUT, "A Gate");
        configOutput(PhasorProbability::OUTB_PHASE_OUTPUT, "B Phase");
        configOutput(PhasorProbability::OUTB_GATE_OUTPUT, "B Gate");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {
        for(int i = 0; i < 16; i++)
        {
            outALogic[i] = false;
            outBLogic[i] = false;
        }
    }

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "probMode", json_integer(probMode));
		return rootJ;
	}

    void dataFromJson(json_t *rootJ) override
    {
		json_t *probModeJ = json_object_get(rootJ, "probMode");
		if (probModeJ)
			probMode = json_integer_value(probModeJ);
    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorProbability::process(const ProcessArgs &args)
{
    const float probKnob = params[PROB_PARAM].getValue();
    const float probDepth = params[PROBCV_PARAM].getValue();

    if (probButtonTrigger.process(params[PROBBUTTON_PARAM].getValue()))
    {
        probMode = (probMode + 1) % 3;
    }

    int polyChannels = getMaxInputPolyphony();

    outputs[OUTA_PHASE_OUTPUT].setChannels(polyChannels);
    outputs[OUTA_GATE_OUTPUT].setChannels(polyChannels);
    outputs[OUTB_PHASE_OUTPUT].setChannels(polyChannels);
    outputs[OUTB_GATE_OUTPUT].setChannels(polyChannels);

    for (int i = 0; i < polyChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        if (resetDetectors[i].detectSimpleReset(inputs[PHASOR_INPUT].getPolyVoltage(i)))
        {
            float probability = ((inputs[PROBCV_INPUT].getPolyVoltage(i)) * probDepth) + probKnob;
            probability = clamp(probability, 0.0f, 1.0f);

            switch(probMode)
            {
                case 0: //probability mode
                outBLogic[i] = randomGen.nextProbability(probability);
                outALogic[i] = !outBLogic[i];
                break;

                case 1: //alternating mode
                if(randomGen.nextProbability(probability))
                {
                    outBLogic[i] = !outBLogic[i]; //flip B's state
                    outALogic[i] = !outBLogic[i]; //then make A opposite
                }
                break;

                case 2: //independent mode
                outALogic[i] = randomGen.nextProbability(probability);
                outBLogic[i] = randomGen.nextProbability(probability);
                break;

                default:
                break;
            }
        }

        float phasorOut = normalizedPhasor * HCV_PHZ_UPSCALE;
        float phasorGate = normalizedPhasor < 0.5f ? HCV_GATE_MAG : 0.0;
        float aPhaseOut = outALogic[i] ? phasorOut : 0.0f;
        float bPhaseOut = outBLogic[i] ? phasorOut : 0.0f;
        float aGateOut = outALogic[i] ? phasorGate : 0.0f;
        float bGateOut = outBLogic[i] ? phasorGate : 0.0f;

        outputs[OUTA_PHASE_OUTPUT].setVoltage(aPhaseOut, i);
        outputs[OUTA_GATE_OUTPUT].setVoltage(aGateOut, i);

        outputs[OUTB_PHASE_OUTPUT].setVoltage(bPhaseOut, i);
        outputs[OUTB_GATE_OUTPUT].setVoltage(bGateOut, i);
    }

    lights[OUTA_PHASE_LIGHT].setBrightness(outputs[OUTA_PHASE_OUTPUT].getVoltage(0) * 0.1f);
    lights[OUTA_GATE_LIGHT].setBrightness(outputs[OUTA_GATE_OUTPUT].getVoltage(0));
    lights[OUTB_PHASE_LIGHT].setBrightness(outputs[OUTB_PHASE_OUTPUT].getVoltage(0) * 0.1f);
    lights[OUTB_GATE_LIGHT].setBrightness(outputs[OUTB_GATE_OUTPUT].getVoltage(0));

    for (int i = 0; i < 3; i++)
    {
        lights[PROBMODE1_LIGHT + i].setBrightness(probMode == i ? 5.0f : 0.0f);
    }
    
}

struct PhasorProbabilityWidget : HCVModuleWidget { PhasorProbabilityWidget(PhasorProbability *module); };

PhasorProbabilityWidget::PhasorProbabilityWidget(PhasorProbability *module)
{
    setSkinPath("res/PhasorProbability.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    createInputPort(78, 73, PhasorProbability::PHASOR_INPUT);
    createParamComboHorizontal(10, 124, PhasorProbability::PROB_PARAM, PhasorProbability::PROBCV_PARAM, PhasorProbability::PROBCV_INPUT);

    int phaseJackY = 270;
    int gateJackY = 310;
    int outAX = 45;
    int outBX = 110;

    createHCVRedLightForJack(outAX, phaseJackY, PhasorProbability::OUTA_PHASE_LIGHT);
    createOutputPort(outAX, phaseJackY, PhasorProbability::OUTA_PHASE_OUTPUT);
    createHCVRedLightForJack(outAX, gateJackY, PhasorProbability::OUTA_GATE_LIGHT);
    createOutputPort(outAX, gateJackY, PhasorProbability::OUTA_GATE_OUTPUT);

    createHCVGreenLightForJack(outBX, phaseJackY, PhasorProbability::OUTB_PHASE_LIGHT);
    createOutputPort(outBX, phaseJackY, PhasorProbability::OUTB_PHASE_OUTPUT);
    createHCVGreenLightForJack(outBX, gateJackY, PhasorProbability::OUTB_GATE_LIGHT);
    createOutputPort(outBX, gateJackY, PhasorProbability::OUTB_GATE_OUTPUT);

    int buttonY = 214;
    createHCVButtonLarge(37, buttonY, PhasorProbability::PROBBUTTON_PARAM);

    for (int i = 0; i < 3; i++)
    {
        int lightY = 200 + (i * 13);
        createHCVRedLight(100, lightY, PhasorProbability::PROBMODE1_LIGHT + i);
    }
    
}

Model *modelPhasorProbability = createModel<PhasorProbability, PhasorProbabilityWidget>("PhasorProbability");
