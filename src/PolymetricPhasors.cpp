#include "HetrickCV.hpp"
#include "DSP/Phasors/HCVPhasorEffects.h"

struct PolymetricPhasors : HCVModule
{
	enum ParamIds
	{
        INSTEPS_PARAM, INSTEPS_SCALE_PARAM,
        OUTSTEPS1_PARAM, 
        OUTSTEPS2_PARAM, 
        OUTSTEPS3_PARAM, 
        OUTSTEPS1_SCALE_PARAM,
        OUTSTEPS2_SCALE_PARAM,
        OUTSTEPS3_SCALE_PARAM,
        RESET1_PARAM, RESET2_PARAM, RESET3_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        INSTEPS_CV_INPUT,
        OUTSTEPS1_CV_INPUT,
        OUTSTEPS2_CV_INPUT,
        OUTSTEPS3_CV_INPUT,
        PHASOR_INPUT,
        RESET1_INPUT, RESET2_INPUT, RESET3_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        PHASOR1_OUTPUT,
        PHASOR2_OUTPUT,
        PHASOR3_OUTPUT,
        FINISH1_OUTPUT,
        FINISH2_OUTPUT,
        FINISH3_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        FINISH1_LIGHT,
        FINISH2_LIGHT,
        FINISH3_LIGHT,
        PHASOR1_LIGHT,
        PHASOR2_LIGHT,
        PHASOR3_LIGHT,
        NUM_LIGHTS
	};


    HCVPhasorDivMult divMults[3][16];
    HCVPhasorResetDetector resetDetectors[3][16];
    dsp::SchmittTrigger resetTriggers[3][16];

    const float MAX_NUM_PULSES = 64.0f;
    const float PULSE_CV_SCALAR = MAX_NUM_PULSES/5.0f;

	PolymetricPhasors()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(INSTEPS_PARAM, 1, MAX_NUM_PULSES, 4, "Input Steps");
        paramQuantities[INSTEPS_PARAM]->snapEnabled = true;
		configParam(INSTEPS_SCALE_PARAM, -1.0, 1.0, 1.0, "Input Steps CV Depth");

        configInput(PHASOR_INPUT, "Phasor");
        configInput(INSTEPS_CV_INPUT, "In Steps CV");

        for (int i = 0; i < 3; i++)
        {
            const auto outChan = std::to_string(i + 1);
            configParam(OUTSTEPS1_PARAM + i, 1, MAX_NUM_PULSES, 4, "Output " + outChan + " Steps");
            paramQuantities[OUTSTEPS1_PARAM + i]->snapEnabled = true;
		    configParam(OUTSTEPS1_SCALE_PARAM + i, -1.0, 1.0, 1.0, "Output " + outChan + " Steps CV Depth");

            configButton(RESET1_PARAM + i, "Reset " + outChan);
            configInput(RESET1_INPUT + i, "Reset " + outChan);
            configInput(OUTSTEPS1_CV_INPUT + i, "Out Steps " + outChan + " CV");
            configOutput(PHASOR1_OUTPUT + i, "Phasor " + outChan);
            configOutput(FINISH1_OUTPUT + i, "Finish " + outChan);
        }

		onReset();
	}

	void process(const ProcessArgs &args) override;

    void onReset() override
    {

	}
    void onRandomize() override
    {
        
    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PolymetricPhasors::process(const ProcessArgs &args)
{
    const int numChannels = setupPolyphonyForAllOutputs();

    const float inStepsKnob = params[INSTEPS_PARAM].getValue();
    const float inStepsCVDepth = params[INSTEPS_SCALE_PARAM].getValue();

    const float outSteps1Knob       = params[OUTSTEPS1_PARAM].getValue();
    const float outSteps1CVDepth    = params[OUTSTEPS1_SCALE_PARAM].getValue();
    const float outSteps2Knob       = params[OUTSTEPS2_PARAM].getValue();
    const float outSteps2CVDepth    = params[OUTSTEPS2_SCALE_PARAM].getValue();
    const float outSteps3Knob       = params[OUTSTEPS3_PARAM].getValue();
    const float outSteps3CVDepth    = params[OUTSTEPS3_SCALE_PARAM].getValue();
    const float outStepsKnobs[3] = {outSteps1Knob, outSteps2Knob, outSteps3Knob};
    const float outStepsCVDepths[3] = {outSteps1CVDepth, outSteps2CVDepth, outSteps3CVDepth};

    const float reset1Button = params[RESET1_PARAM].getValue();
    const float reset2Button = params[RESET2_PARAM].getValue();
    const float reset3Button = params[RESET3_PARAM].getValue();
    const float resetButtons[3] = {reset1Button, reset2Button, reset3Button};

    for (int chan = 0; chan < numChannels; chan++)
    {
        float modulatedInPulses = inStepsCVDepth * inputs[INSTEPS_CV_INPUT].getPolyVoltage(chan) * PULSE_CV_SCALAR;
        float inPulses = clamp(inStepsKnob + modulatedInPulses, 1.0f, MAX_NUM_PULSES);

        float normalizedPhasor = scaleAndWrapPhasor(inputs[PHASOR_INPUT].getPolyVoltage(chan));

        for (int outSet = 0; outSet < 3; outSet++)
        {
            float modulatedOutPulses = outStepsCVDepths[outSet] * inputs[OUTSTEPS1_CV_INPUT + outSet].getPolyVoltage(chan) * PULSE_CV_SCALAR;
            float outPulses = clamp(outStepsKnobs[outSet] + modulatedOutPulses, 1.0f, MAX_NUM_PULSES);

            divMults[outSet][chan].setDivider(outPulses);
            divMults[outSet][chan].setMultiplier(inPulses);

            float channelReset = inputs[RESET1_INPUT + outSet].getPolyVoltage(chan);
            
            if(resetTriggers[outSet][chan].process(channelReset + resetButtons[outSet]))
            {
                divMults[outSet][chan].reset();
            }

            float speedPhasor = divMults[outSet][chan].basicSync(normalizedPhasor);
            outputs[PHASOR1_OUTPUT + outSet].setVoltage(speedPhasor * HCV_PHZ_UPSCALE, chan);
            bool resetGate = resetDetectors[outSet][chan].detectProportionalReset(speedPhasor);
            outputs[FINISH1_OUTPUT + outSet].setVoltage( resetGate ? HCV_PHZ_GATESCALE : 0.0f, chan);
        }

    }

    setLightFromOutput(PHASOR1_LIGHT, PHASOR1_OUTPUT);
    setLightFromOutput(PHASOR2_LIGHT, PHASOR2_OUTPUT);
    setLightFromOutput(PHASOR3_LIGHT, PHASOR3_OUTPUT);

    setLightSmoothFromOutput(FINISH1_LIGHT, FINISH1_OUTPUT);
    setLightSmoothFromOutput(FINISH2_LIGHT, FINISH2_OUTPUT);
    setLightSmoothFromOutput(FINISH3_LIGHT, FINISH3_OUTPUT);
}

struct PolymetricPhasorsWidget : HCVModuleWidget { PolymetricPhasorsWidget(PolymetricPhasors *module); };

PolymetricPhasorsWidget::PolymetricPhasorsWidget(PolymetricPhasors *module)
{
    setSkinPath("res/PolymetricPhasors.svg");
    initializeWidget(module);
    
    const float knobY = 60.0f;
    const float knobX = 70.0f;
    const float spacing = 50.0f;

    createParamComboHorizontal(knobX, knobY,                PolymetricPhasors::OUTSTEPS1_PARAM, PolymetricPhasors::OUTSTEPS1_SCALE_PARAM, PolymetricPhasors::OUTSTEPS1_CV_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing,      PolymetricPhasors::OUTSTEPS2_PARAM, PolymetricPhasors::OUTSTEPS2_SCALE_PARAM, PolymetricPhasors::OUTSTEPS2_CV_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0,  PolymetricPhasors::OUTSTEPS3_PARAM, PolymetricPhasors::OUTSTEPS3_SCALE_PARAM, PolymetricPhasors::OUTSTEPS3_CV_INPUT);

    //////PARAMS//////
    createParamComboVertical(15, knobY, PolymetricPhasors::INSTEPS_PARAM, PolymetricPhasors::INSTEPS_SCALE_PARAM, PolymetricPhasors::INSTEPS_CV_INPUT);


    //////INPUTS//////
    
    int topJackY = 209;
    int middleJackY = 265;
    int bottomJackY = 319;

    int spacing1 = 56;
    int jackX = 22;
    createInputPort(jackX, topJackY, PolymetricPhasors::PHASOR_INPUT);
    createInputPort(jackX + spacing1, topJackY, PolymetricPhasors::RESET1_INPUT);
    createInputPort(jackX + spacing1*2, topJackY, PolymetricPhasors::RESET2_INPUT);
    createInputPort(jackX + spacing1*3, topJackY, PolymetricPhasors::RESET3_INPUT);

    //createHCVButtonSmallForJack(jackX + spacing1, topJackY, PolymetricPhasors::RESET1_PARAM);
    //createHCVButtonSmallForJack(jackX + spacing1*2, topJackY, PolymetricPhasors::RESET2_PARAM);
    //createHCVButtonSmallForJack(jackX + spacing1*3, topJackY, PolymetricPhasors::RESET3_PARAM);

    int spacing2 = 57;
    int leftX = 53;
    int middleX = leftX + spacing2;
    int rightX = leftX + spacing2*2;
    
    createOutputPort(leftX, middleJackY, PolymetricPhasors::FINISH1_OUTPUT);
    createOutputPort(middleX, middleJackY, PolymetricPhasors::FINISH2_OUTPUT);
    createOutputPort(rightX, middleJackY, PolymetricPhasors::FINISH3_OUTPUT);
    createOutputPort(leftX, bottomJackY, PolymetricPhasors::PHASOR1_OUTPUT);
    createOutputPort(middleX, bottomJackY, PolymetricPhasors::PHASOR2_OUTPUT);
    createOutputPort(rightX, bottomJackY, PolymetricPhasors::PHASOR3_OUTPUT);

    createHCVRedLightForJack(leftX, middleJackY, PolymetricPhasors::FINISH1_LIGHT);
    createHCVRedLightForJack(middleX, middleJackY, PolymetricPhasors::FINISH2_LIGHT);
    createHCVRedLightForJack(rightX, middleJackY, PolymetricPhasors::FINISH3_LIGHT);

    createHCVRedLightForJack(leftX, bottomJackY, PolymetricPhasors::PHASOR1_LIGHT);
    createHCVRedLightForJack(middleX, bottomJackY, PolymetricPhasors::PHASOR2_LIGHT);
    createHCVRedLightForJack(rightX, bottomJackY, PolymetricPhasors::PHASOR3_LIGHT);
    
}

Model *modelPolymetricPhasors = createModel<PolymetricPhasors, PolymetricPhasorsWidget>("PolymetricPhasors");
