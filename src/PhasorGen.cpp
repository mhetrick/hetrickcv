#include "HetrickCV.hpp"
#include "DSP/HCVChaos.h" 
#include "DSP/HCVDCFilter.h"
#include "DSP/HCVSampleRate.h"
#include "DSP/HCVCrackle.h"
#include "DSP/HCVPhasor.h"

struct PhasorGen : HCVModule
{
	enum ParamIds
	{
		FREQ_PARAM, FREQ_SCALE_PARAM,

        PHASE_PARAM, PHASE_SCALE_PARAM,
        PW_PARAM, PW_SCALE_PARAM,
        PULSES_PARAM, PULSES_SCALE_PARAM,
        JITTER_PARAM, JITTER_SCALE_PARAM,

        RANGE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        CLOCK_INPUT,
        FM_INPUT,

        PHASE_INPUT,
        PW_INPUT,
        PULSES_INPUT,
        JITTER_INPUT,
        
		NUM_INPUTS
	};
	enum OutputIds
	{
		PHASOR_OUTPUT,
        PULSES_OUTPUT,
        JITTER_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        NUM_LIGHTS = 8
	};

	PhasorGen()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PhasorGen::FREQ_PARAM, 0.01, 1.0, 1.0, "Sample Rate");
		configParam(PhasorGen::FREQ_SCALE_PARAM, -1.0, 1.0, 1.0, "Sample Rate CV Depth");

        configParam(PhasorGen::PHASE_PARAM, -5.0, 5.0, 0.0, "Phase");
		configParam(PhasorGen::PHASE_SCALE_PARAM, -1.0, 1.0, 1.0, "Phase CV Depth");

        configParam(PhasorGen::PW_PARAM, -5.0, 5.0, 0.0, "Pulse Width");
		configParam(PhasorGen::PW_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulse Width CV Depth");

        configSwitch(PhasorGen::PULSES_PARAM, 1, 64.0, 1, "Pulses Per Cycle");
        paramQuantities[PULSES_PARAM]->snapEnabled = true;
		configParam(PhasorGen::PULSES_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulses Per Cycle CV Depth");

        configParam(PhasorGen::JITTER_PARAM, -5.0, 5.0, 0.0, "Jitter");
		configParam(PhasorGen::JITTER_SCALE_PARAM, -1.0, 1.0, 1.0, "Jitter CV Depth");

        configSwitch(PhasorGen::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});

        configInput(CLOCK_INPUT, "Clock");
        configInput(FM_INPUT, "Frequency CV");

        configInput(PHASE_INPUT, "Index Multiplier CV");
        configInput(PW_INPUT, "Phase Increment CV");
        configInput(PULSES_INPUT, "Phase Multiplier CV");
        configInput(JITTER_INPUT, "Feedback CV CV");

        configOutput(PHASOR_OUTPUT, "Phasor");
        configOutput(PULSES_OUTPUT, "Pulses");
        configOutput(JITTER_OUTPUT, "Jitter");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    HCVPhasor phasor;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorGen::process(const ProcessArgs &args)
{
    bool lfoMode = params[RANGE_PARAM].getValue() == 0.0f;
    phasor.enableLFOMode(lfoMode);

    const float pulseWidth = getNormalizedModulatedValue(PW_PARAM, PW_INPUT, PW_SCALE_PARAM);
    phasor.setPulseWidth(pulseWidth);

    const int pulses = params[PULSES_PARAM].getValue();
    phasor.setPulsesPerCycle(pulses);

    phasor.setFreqDirect(2.0f);

    const float jitter = getNormalizedModulatedValue(JITTER_PARAM, JITTER_INPUT, JITTER_SCALE_PARAM);
    phasor.setJitterDepth(jitter);

    outputs[PHASOR_OUTPUT].setVoltage(phasor());
    outputs[PULSES_OUTPUT].setVoltage(phasor.getPulse()); 
    outputs[JITTER_OUTPUT].setVoltage(phasor.getJitterSample() * 5.0f);   
}


struct PhasorGenWidget : HCVModuleWidget { PhasorGenWidget(PhasorGen *module); };

PhasorGenWidget::PhasorGenWidget(PhasorGen *module)
{
	setSkinPath("res/PhasorGen.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 35.0f;
    const float knobX = 70.0f;
    const float spacing = 45.0f;

    createParamComboHorizontal(knobX, knobY,                PhasorGen::PHASE_PARAM, PhasorGen::PHASE_SCALE_PARAM, PhasorGen::PHASE_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing,      PhasorGen::PW_PARAM, PhasorGen::PW_SCALE_PARAM, PhasorGen::PW_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0,  PhasorGen::PULSES_PARAM, PhasorGen::PULSES_SCALE_PARAM, PhasorGen::PULSES_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*3.0,  PhasorGen::JITTER_PARAM, PhasorGen::JITTER_SCALE_PARAM, PhasorGen::JITTER_INPUT);

    const float sRateX = 15.0f;
    const float sRateY = 60.0f;
    createParamComboVertical(sRateX, sRateY, PhasorGen::FREQ_PARAM, PhasorGen::FREQ_SCALE_PARAM, PhasorGen::FM_INPUT);

    const float switchY = 238.0f;
    createHCVSwitchVert(19.0f, switchY, PhasorGen::RANGE_PARAM);

    const float jackY = 305.0f;
	//////INPUTS//////
    //createInputPort(50.0f, jackY, PhasorGen::CLOCK_INPUT);

	//////OUTPUTS//////
    createOutputPort(50.0f, jackY, PhasorGen::PHASOR_OUTPUT);
    createOutputPort(134.0f, jackY, PhasorGen::PULSES_OUTPUT);
    createOutputPort(184.0f, jackY, PhasorGen::JITTER_OUTPUT);
}

Model *modelPhasorGen = createModel<PhasorGen, PhasorGenWidget>("PhasorGen");
