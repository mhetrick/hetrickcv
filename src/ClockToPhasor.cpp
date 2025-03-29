#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h" 
#include "DSP/Phasors/HCVPhasor.h"
#include "dsp/approx.hpp"

struct ClockToPhasor : HCVModule
{
	enum ParamIds
	{
        PULSES_PARAM, PULSES_SCALE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        PULSES_INPUT,
        CLOCK_INPUT,
        RESET_INPUT,
        
		NUM_INPUTS
	};
	enum OutputIds
	{
		PHASOR_OUTPUT,
        FINISH_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        PHASOR_LIGHT,
        FINISH_LIGHT,
        NUM_LIGHTS
	};

    const float MAX_NUM_PULSES = 64.0f;
    const float PULSE_CV_SCALAR = MAX_NUM_PULSES/5.0f;

	ClockToPhasor()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(ClockToPhasor::PULSES_PARAM, 1, MAX_NUM_PULSES, 4, "Pulses Per Cycle");
        paramQuantities[PULSES_PARAM]->snapEnabled = true;
		configParam(ClockToPhasor::PULSES_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulses Per Cycle CV Depth");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");

        configOutput(PHASOR_OUTPUT, "Phasor");
        configOutput(FINISH_OUTPUT, "Finished Trigger");
	}

	void process(const ProcessArgs &args) override;

    HCVPhasor phasors[16];
    HCVClockSync clockSyncs[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void ClockToPhasor::process(const ProcessArgs &args)
{
    const float pulsesKnob = params[PULSES_PARAM].getValue();
    const float pulsesCVDepth = params[PULSES_SCALE_PARAM].getValue();

    int numChannels = setupPolyphonyForAllOutputs();

    for (int i = 0; i < numChannels; i++)
    {
        //sync to incoming clock
        clockSyncs[i].processGateClockInput(inputs[CLOCK_INPUT].getPolyVoltage(i));
        const float baseClockFreq = clockSyncs[i].getBaseClockFreq();

        //then scale by Pulses per cycle
        float modulatedPulses = pulsesCVDepth * inputs[PULSES_INPUT].getPolyVoltage(i) * PULSE_CV_SCALAR;
        float pulses = clamp(pulsesKnob + modulatedPulses, 1.0f, MAX_NUM_PULSES);

        float freq = baseClockFreq / pulses;

        phasors[i].setFreqDirect(freq);

        bool resetThisFrame = phasors[i].processGateResetInput(inputs[RESET_INPUT].getPolyVoltage(i));

        outputs[PHASOR_OUTPUT].setVoltage(phasors[i](), i);
        bool finishHigh = phasors[i].phasorFinishedThisSample();
        outputs[FINISH_OUTPUT].setVoltage( finishHigh ? HCV_GATE_MAG : 0.0f, i);
    }

    setLightFromOutput(PHASOR_LIGHT, PHASOR_OUTPUT);
    setLightSmoothFromOutput(FINISH_LIGHT, FINISH_OUTPUT);
}


struct ClockToPhasorWidget : HCVModuleWidget { ClockToPhasorWidget(ClockToPhasor *module); };

ClockToPhasorWidget::ClockToPhasorWidget(ClockToPhasor *module)
{
	setSkinPath("res/ClockToPhasor.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 90.0f;
    const float knobX = 70.0f;

    createParamComboVertical(40, knobY, ClockToPhasor::PULSES_PARAM, ClockToPhasor::PULSES_SCALE_PARAM, ClockToPhasor::PULSES_INPUT);

    
    int leftX = 21;
    int rightX = 76;
    int topJackY = 245;
    int bottomJackY = 310;

	//////INPUTS//////
    createInputPort(leftX, topJackY, ClockToPhasor::CLOCK_INPUT);
    createInputPort(rightX, topJackY, ClockToPhasor::RESET_INPUT);

	//////OUTPUTS//////
    createOutputPort(leftX, bottomJackY, ClockToPhasor::PHASOR_OUTPUT);
    createOutputPort(rightX, bottomJackY, ClockToPhasor::FINISH_OUTPUT);

    createHCVRedLightForJack(leftX, bottomJackY, ClockToPhasor::PHASOR_LIGHT);
    createHCVRedLightForJack(rightX, bottomJackY, ClockToPhasor::FINISH_LIGHT);
}

Model *modelClockToPhasor = createModel<ClockToPhasor, ClockToPhasorWidget>("ClockToPhasor");
