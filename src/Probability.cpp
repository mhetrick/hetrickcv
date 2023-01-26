#include "HetrickCV.hpp"
#include "DSP/HCVRandom.h"

struct Probability : HCVModule
{
	enum ParamIds
	{
        PROB_PARAM,
        PROBCV_PARAM,
        PROBBUTTON_PARAM,
        OUTBUTTON_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        GATE_INPUT,
        PROBCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUTA_OUTPUT,
        OUTB_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        OUTA_LIGHT,
        OUTB_LIGHT,

        PROBMODE1_LIGHT,
        PROBMODE2_LIGHT,
        PROBMODE3_LIGHT,

        OUTMODE1_LIGHT,
        OUTMODE2_LIGHT,
        OUTMODE3_LIGHT,

        NUM_LIGHTS
	};

    dsp::SchmittTrigger probButtonTrigger, outButtonTrigger;
    dsp::SchmittTrigger clockTrigger[16];
    HCVTriggerGenerator triggerA[16], triggerB[16];
    bool outALogic[16];
    bool outBLogic[16];
    int probMode = 0;
    int outMode = 2;

    HCVRandom randomGen;

	Probability()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(Probability::PROB_PARAM, 0.0, 1.0, 0.5, "Probability");
        configParam(Probability::PROBCV_PARAM, -1.0, 1.0, 1.0, "Probability CV Depth");

        configButton(Probability::PROBBUTTON_PARAM, "Probability Mode");
        configButton(Probability::OUTBUTTON_PARAM, "Output Mode");

        configInput(Probability::GATE_INPUT, "Gate");
        configInput(Probability::PROBCV_INPUT, "Probability CV");

        configOutput(Probability::OUTA_OUTPUT, "A");
        configOutput(Probability::OUTB_OUTPUT, "B");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {
        for(int i = 0; i < 16; i++)
        {
            outALogic[i] = false;
            outBLogic[i] = false;
            clockTrigger[i].reset();
            triggerA[i].reset();
            triggerB[i].reset();
        }
    }

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "probMode", json_integer(probMode));
        json_object_set_new(rootJ, "outMode", json_integer(outMode));
		return rootJ;
	}

    void dataFromJson(json_t *rootJ) override
    {
		json_t *probModeJ = json_object_get(rootJ, "probMode");
		if (probModeJ)
			probMode = json_integer_value(probModeJ);

        json_t *outModeJ = json_object_get(rootJ, "outMode");
		if (outModeJ)
			outMode = json_integer_value(outModeJ);
    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Probability::process(const ProcessArgs &args)
{
    const float probKnob = params[PROB_PARAM].getValue();
    const float probDepth = params[PROBCV_PARAM].getValue();

    if (probButtonTrigger.process(params[PROBBUTTON_PARAM].getValue()))
    {
        probMode = (probMode + 1) % 3;
    }
    
    if (outButtonTrigger.process(params[OUTBUTTON_PARAM].getValue()))
    {
        outMode = (outMode + 1) % 3;
    }

    outputs[OUTA_OUTPUT].setChannels(getMaxInputPolyphony());
    outputs[OUTB_OUTPUT].setChannels(getMaxInputPolyphony());

    for (int i = 0; i < getMaxInputPolyphony(); i++)
    {
        if (clockTrigger[i].process(inputs[GATE_INPUT].getPolyVoltage(i)))
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

            if(outMode == 0)
            {
                if(outALogic[i]) triggerA[i].trigger();
                if(outBLogic[i]) triggerB[i].trigger(); 
            }
        }

        float aOut = outALogic[i] ? 5.0f : 0.0f;
        float bOut = outBLogic[i] ? 5.0f : 0.0f;
        bool clockHigh = inputs[GATE_INPUT].getPolyVoltage(i) > 0.9f;

        switch(outMode)
        {
            case 0: //trigger mode
            outputs[OUTA_OUTPUT].setVoltage(triggerA[i].process() ? 5.0f : 0.0f, i);
            outputs[OUTB_OUTPUT].setVoltage(triggerB[i].process() ? 5.0f : 0.0f, i);
            break;

            case 1: //hold mode
            outputs[OUTA_OUTPUT].setVoltage(aOut, i);
            outputs[OUTB_OUTPUT].setVoltage(bOut, i);
            break;

            case 2:
            outputs[OUTA_OUTPUT].setVoltage(clockHigh ? aOut : 0.0f, i);
            outputs[OUTB_OUTPUT].setVoltage(clockHigh ? bOut : 0.0f, i);
            break;

            default:
            break;
        }
    }

    lights[OUTA_LIGHT].setBrightnessSmooth(outputs[OUTA_OUTPUT].getVoltage(0), args.sampleTime * 4.0f);
    lights[OUTB_LIGHT].setBrightnessSmooth(outputs[OUTB_OUTPUT].getVoltage(0), args.sampleTime * 4.0f);

    for (int i = 0; i < 3; i++)
    {
        lights[PROBMODE1_LIGHT + i].setBrightness(probMode == i ? 5.0f : 0.0f);
        lights[OUTMODE1_LIGHT + i].setBrightness(outMode == i ? 5.0f : 0.0f);
    }
    
}

struct ProbabilityWidget : HCVModuleWidget { ProbabilityWidget(Probability *module); };

ProbabilityWidget::ProbabilityWidget(Probability *module)
{
    setSkinPath("res/Probability.svg");
    initializeWidget(module);
    
    //////PARAMS//////

    //////INPUTS//////
    createInputPort(78, 73, Probability::GATE_INPUT);
    createParamComboHorizontal(10, 124, Probability::PROB_PARAM, Probability::PROBCV_PARAM, Probability::PROBCV_INPUT);
    
    createHCVRedLight(80, 320, Probability::OUTA_LIGHT);
    createHCVGreenLight(95, 320, Probability::OUTB_LIGHT);

    createOutputPort(45, 310, Probability::OUTA_OUTPUT);
    createOutputPort(110, 310, Probability::OUTB_OUTPUT);

    int buttonY = 214;
    createHCVButtonLarge(37, buttonY, Probability::PROBBUTTON_PARAM);
    createHCVButtonLarge(121, buttonY, Probability::OUTBUTTON_PARAM);

    for (int i = 0; i < 3; i++)
    {
        int lightY = 249 + (i * 13);
        createHCVRedLight(27, lightY, Probability::PROBMODE1_LIGHT + i);
        createHCVGreenLight(147, lightY, Probability::OUTMODE1_LIGHT + i);
    }
    
}

Model *modelProbability = createModel<Probability, ProbabilityWidget>("Probability");
