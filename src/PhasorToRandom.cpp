#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PhasorToRandom : HCVModule
{
	enum ParamIds
	{
        STEPS_PARAM,
        STEPSCV_PARAM,
        PROB_PARAM,
        PROBCV_PARAM,
        RANGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
        STEPSCV_INPUT,
        PROBCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        STEPPED_OUTPUT,
        SLEWED_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        ENUMS(STEP_LIGHT, 2),
        ENUMS(SLEW_LIGHT, 2),
        NUM_LIGHTS
	};

    static constexpr float MAX_STEPS = 64.0f;
    static constexpr float STEPS_CV_SCALE = MAX_STEPS/5.0f;

    HCVRandom randomGenerator;
    HCVPhasorStepDetector stepDetectors[16];
    std::vector<std::vector<float>> randomVoltageArray;

	PhasorToRandom()
	{
        randomVoltageArray.resize(16);
        for(int channel = 0; channel < 16; channel++)
        {
            randomVoltageArray[channel].resize(MAX_STEPS);
            for(int i = 0; i < MAX_STEPS; i++)
            {
                randomVoltageArray[channel][i] = randomGenerator.nextFloat() * 10.0f;
            }
        }

        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(STEPS_PARAM, 1.0, MAX_STEPS, 16.0, "Steps");
		configParam(STEPSCV_PARAM, -1.0, 1.0, 1.0, "Steps CV Depth");
        paramQuantities[STEPS_PARAM]->snapEnabled = true;

        configParam(PROB_PARAM, -5.0, 5.0, 1.0, "Probability");
        configParam(PROBCV_PARAM, -1.0, 1.0, 1.0, "Probability CV Depth");

        configSwitch(RANGE_PARAM, 0.0, 1.0, 1.0, "Voltage Range", {"+/- 5V", "0-10V"});

        configInput(PHASOR_INPUT, "Phasor");
        configInput(STEPSCV_INPUT, "Steps CV");
        configInput(PROBCV_INPUT, "Probability CV");

        configOutput(STEPPED_OUTPUT, "Stepped Random");
        configOutput(SLEWED_OUTPUT, "Slewed Random");

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {

    }


    //TODO: Save random voltage array to JSON.
    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
        json_t *randomDataJSONArray = json_array();

        // Save the random voltage array to JSON
        for(int i = 0; i < 16; i++)
        {
            json_t *channelArray = json_array();
            for (int j = 0; j < MAX_STEPS; j++)
            {
                json_array_append_new(channelArray, json_real(randomVoltageArray[i][j]));
            }
            json_array_append_new(randomDataJSONArray, channelArray);
        }

        json_object_set_new(rootJ, "randomVoltages", randomDataJSONArray);
		return rootJ;
	}
    void dataFromJson(json_t *rootJ) override
    {
		json_t *voltageData = json_object_get(rootJ, "randomVoltages");

        if(voltageData)
        {
            for(int i = 0; i < 16; i++)
            {
                json_t *channelArray = json_array_get(voltageData, i);
                if (channelArray)
                {
                    for (int j = 0; j < MAX_STEPS; j++)
                    {
                        json_t *value = json_array_get(channelArray, j);
                        if (value)
                        {
                            randomVoltageArray[i][j] = json_real_value(value);
                        }
                    }
                }
            }
        }
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorToRandom::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();

    float stepsKnob = params[STEPS_PARAM].getValue();
    float stepsCVDepth = params[STEPSCV_PARAM].getValue() * STEPS_CV_SCALE;

    const float probabilityKnob = params[PROB_PARAM].getValue();
    const float probabilityDepth = params[PROBCV_PARAM].getValue();

    const bool voltageRange = params[RANGE_PARAM].getValue() > 0.0f;
    const float voltageOffset = voltageRange ? 0.0f : -5.0f;
    const float lightScale = voltageRange ? 0.1f : 0.2f;

    for (int channel = 0; channel < numChannels; channel++)
    {
        float steps = stepsKnob + (stepsCVDepth * inputs[STEPSCV_INPUT].getPolyVoltage(channel));
        steps = floorf(clamp(steps, 1.0f, MAX_STEPS));
        stepDetectors[channel].setNumberSteps(steps);

        float probability = probabilityKnob + (probabilityDepth * inputs[PROBCV_INPUT].getPolyVoltage(channel));
        probability = clamp(probability, -5.0f, 5.0f) * 0.1f + 0.5f;

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(channel));

        const bool stepAdvanced = stepDetectors[channel](normalizedPhasor);
        const int currentStep = stepDetectors[channel].getCurrentStep();
        const int nextStep = (currentStep + 1) % int(steps);

        if(stepAdvanced)
        {
            if(randomGenerator.nextProbability(probability))
            {
                randomVoltageArray[channel][nextStep] = randomGenerator.nextFloat() * 10.0f;
            }
        }

        const float fractionalStep = stepDetectors[channel].getFractionalStep();

        const float steppedVoltage = randomVoltageArray[channel][currentStep];
        const float slewedVoltage = LERP(fractionalStep, randomVoltageArray[channel][nextStep], steppedVoltage);

        outputs[STEPPED_OUTPUT].setVoltage(steppedVoltage + voltageOffset, channel);
        outputs[SLEWED_OUTPUT].setVoltage(slewedVoltage + voltageOffset, channel);
    }

    setBipolarLightBrightness(STEP_LIGHT, outputs[STEPPED_OUTPUT].getVoltage() * lightScale);
    setBipolarLightBrightness(SLEW_LIGHT, outputs[SLEWED_OUTPUT].getVoltage() * lightScale);
}

struct PhasorToRandomWidget : HCVModuleWidget { PhasorToRandomWidget(PhasorToRandom *module); };

PhasorToRandomWidget::PhasorToRandomWidget(PhasorToRandom *module)
{
    setSkinPath("res/PhasorToRandom.svg");
    initializeWidget(module);
    
    int knobY = 90;

    //////PARAMS//////
    createParamComboVertical(15, knobY, PhasorToRandom::STEPS_PARAM, PhasorToRandom::STEPSCV_PARAM, PhasorToRandom::STEPSCV_INPUT);
    createParamComboVertical(70, knobY, PhasorToRandom::PROB_PARAM, PhasorToRandom::PROBCV_PARAM, PhasorToRandom::PROBCV_INPUT);

    createHCVSwitchVert(89, 252, PhasorToRandom::RANGE_PARAM);

    //////INPUTS//////
    int leftX = 21;
    int rightX = 76;
    int topJackY = 245;
    int bottomJackY = 310;
    createInputPort(leftX, topJackY, PhasorToRandom::PHASOR_INPUT);
    
    createOutputPort(leftX, bottomJackY, PhasorToRandom::STEPPED_OUTPUT);
    createOutputPort(rightX, bottomJackY, PhasorToRandom::SLEWED_OUTPUT);

    createHCVBipolarLightForJack(leftX, bottomJackY, PhasorToRandom::STEP_LIGHT);
    createHCVBipolarLightForJack(rightX, bottomJackY, PhasorToRandom::SLEW_LIGHT);
}

Model *modelPhasorToRandom = createModel<PhasorToRandom, PhasorToRandomWidget>("PhasorToRandom");
