#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h" 
#include "DSP/Phasors/HCVPhasor.h"
#include "dsp/approx.hpp"

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
        FM_INPUT,

        PHASE_INPUT,
        PW_INPUT,
        PULSES_INPUT,
        JITTER_INPUT,

        VOCT_INPUT,

        CLOCK_INPUT,
        RESET_INPUT,
        FREEZE_INPUT,
        
		NUM_INPUTS
	};
	enum OutputIds
	{
		PHASOR_OUTPUT,
        PULSES_OUTPUT,
        JITTER_OUTPUT,
        FINISH_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        PHASOR_LIGHT,
        PULSES_LIGHT,
        JITTER_POS_LIGHT, JITTER_NEG_LIGHT,
        FINISH_LIGHT,
        NUM_LIGHTS
	};

    const float MAX_NUM_PULSES = 64.0f;
    const float PULSE_CV_SCALAR = MAX_NUM_PULSES/5.0f;

	PhasorGen()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		struct FrequencyQuantity : ParamQuantity {
			float getDisplayValue() override {
				PhasorGen* module = reinterpret_cast<PhasorGen*>(this->module);
				if (!module->inputs[CLOCK_INPUT].isConnected()) 
                {
                    unit = " Hz";
                    if(module->params[RANGE_PARAM].getValue() > 0.0f) //oscillator
                    {
                        return bipolarParamToOscillatorFrequencyScalar(getValue());
                    }
                    return bipolarParamToLFOFrequencyScalar(getValue()); //LFO
				}
				
                //Clock Sync
                unit = "x";
                return bipolarParamToClockMultScalar(getValue());
			}
            void setDisplayValue(float displayValue) override
            {
                if(std::isnan(displayValue)) 
                    return;

                auto result = displayValue;

                if (!module->inputs[CLOCK_INPUT].isConnected()) 
                {
                    if(module->params[RANGE_PARAM].getValue() > 0.0f) //oscillator
                    {
                        result = frequencyToBipolarParamUnscalar(result);;
                    }
                    else result = lfoFrequencyToBipolarParamUnscalar(result);; //LFO
				}
				else result = clockMultToBipolarParamUnscalar(result); //Clock Sync

                setImmediateValue(result);
            }
		};
		configParam<FrequencyQuantity>(FREQ_PARAM, -1.f, 1.f, 0.f, "Frequency");
		configParam(PhasorGen::FREQ_SCALE_PARAM, -1.0, 1.0, 1.0, "Cycle Frequency CV Depth");

        configParam(PhasorGen::PHASE_PARAM, -5.0, 5.0, 0.0, "Phase");
		configParam(PhasorGen::PHASE_SCALE_PARAM, -1.0, 1.0, 1.0, "Phase CV Depth");

        configParam(PhasorGen::PW_PARAM, -5.0, 5.0, 0.0, "Pulse Width");
		configParam(PhasorGen::PW_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulse Width CV Depth");

        configParam(PhasorGen::PULSES_PARAM, 1, MAX_NUM_PULSES, 1, "Pulses Per Cycle");
        paramQuantities[PULSES_PARAM]->snapEnabled = true;
		configParam(PhasorGen::PULSES_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulses Per Cycle CV Depth");

        configParam(PhasorGen::JITTER_PARAM, 0.0, 5.0, 0.0, "Jitter");
		configParam(PhasorGen::JITTER_SCALE_PARAM, -1.0, 1.0, 1.0, "Jitter CV Depth");

        configSwitch(PhasorGen::RANGE_PARAM, 0.0, 1.0, 0.0, "Speed Range", {"Slow", "Fast"});

        configInput(FM_INPUT, "Frequency CV");

        configInput(PHASE_INPUT, "Phase CV");
        configInput(PW_INPUT, "Pulse Width CV");
        configInput(PULSES_INPUT, "Pulses Per Cycle CV");
        configInput(JITTER_INPUT, "Jitter CV");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(FREEZE_INPUT, "Freeze");

        configOutput(PHASOR_OUTPUT, "Phasor");
        configOutput(PULSES_OUTPUT, "Pulses");
        configOutput(JITTER_OUTPUT, "Jitter");
        configOutput(FINISH_OUTPUT, "Finished Trigger");

        random::init();
	}

	void process(const ProcessArgs &args) override;

    HCVPhasor phasors[16];
    HCVClockSync clockSyncs[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorGen::process(const ProcessArgs &args)
{
    const float jitterKnob = params[JITTER_PARAM].getValue();
    const float jitterCVDepth = params[JITTER_SCALE_PARAM].getValue();

    const float freqKnob = params[FREQ_PARAM].getValue();
    const float fmCVKnob = params[FREQ_SCALE_PARAM].getValue();

    const float pulsesKnob = params[PULSES_PARAM].getValue();
    const float pulsesCVDepth = params[PULSES_SCALE_PARAM].getValue();

    const float phaseKnob = params[PHASE_PARAM].getValue();
    const float phaseCVDepth = params[PHASE_SCALE_PARAM].getValue();

    const float pwKnob = params[PW_PARAM].getValue();
    const float pwCVDepth = params[PW_SCALE_PARAM].getValue();

    const bool lfoMode = params[RANGE_PARAM].getValue() == 0.0f;

    int numChannels = setupPolyphonyForAllOutputs();

    for (int i = 0; i < numChannels; i++)
    {
        const float jitterCVIn = inputs[JITTER_INPUT].getPolyVoltage(i);
        float jitterDepth = jitterKnob + (jitterCVDepth * jitterCVIn);
        jitterDepth = clamp(jitterDepth, 0.0f, 5.0f) * 0.2f;
        const float jitterValue = phasors[i].getJitterSample() * 5.0f;

        if(inputs[CLOCK_INPUT].isConnected()) //clock mode
        {
            clockSyncs[i].processGateClockInput(inputs[CLOCK_INPUT].getPolyVoltage(i));
            const float baseClockFreq = clockSyncs[i].getBaseClockFreq();

            float pitch =  freqKnob + inputs[VOCT_INPUT].getPolyVoltage(i);
            pitch += (inputs[FM_INPUT].getPolyVoltage(i) * fmCVKnob);
            pitch += (jitterDepth * jitterValue);

            float freq = baseClockFreq * rack::dsp::approxExp2_taylor5(pitch);

            phasors[i].setFreqDirect(freq);
        }
        else //freq mode
        {
            float pitchParamValue = freqKnob;
            
            if(!lfoMode) pitchParamValue = pitchParamValue * 4.5f;
            else pitchParamValue = pitchParamValue * 9.0f + 1.0f;

            float pitch = pitchParamValue + inputs[VOCT_INPUT].getPolyVoltage(i);
            pitch += (inputs[FM_INPUT].getPolyVoltage(i) * fmCVKnob);
            pitch += (jitterDepth * jitterValue);

            float baseFreq = lfoMode ? 1.0f : dsp::FREQ_C4;
            float freq = baseFreq * rack::dsp::approxExp2_taylor5(pitch);

            freq = clamp(freq, 0.f, args.sampleRate / 2.f);
            phasors[i].setFreqDirect(freq);
        }

        float phase = phaseKnob + (phaseCVDepth * inputs[PHASE_INPUT].getPolyVoltage(i));
        phase = clamp(phase, -5.0f, 5.0f) * 0.2f;
        phasors[i].setPhaseOffset(phase);

        float pulseWidth = pwKnob + (pwCVDepth * inputs[PW_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;;
        phasors[i].setPulseWidth(pulseWidth);

        phasors[i].setFrozen(inputs[FREEZE_INPUT].getPolyVoltage(i) >= 1.0f);
        bool resetThisFrame = phasors[i].processGateResetInput(inputs[RESET_INPUT].getPolyVoltage(i));

        
        float modulatedPulses = pulsesCVDepth * inputs[PULSES_INPUT].getPolyVoltage(i) * PULSE_CV_SCALAR;
        float pulses = clamp(pulsesKnob + modulatedPulses, 1.0f, MAX_NUM_PULSES);
        phasors[i].setPulsesPerCycle(pulses);

        outputs[PHASOR_OUTPUT].setVoltage(phasors[i](), i);
        outputs[PULSES_OUTPUT].setVoltage(phasors[i].getPulse(), i); 
        outputs[JITTER_OUTPUT].setVoltage(jitterValue, i);
        bool finishHigh = phasors[i].phasorFinishedThisSample();
        outputs[FINISH_OUTPUT].setVoltage( finishHigh ? HCV_GATE_MAG : 0.0f, i);
    }

    setLightFromOutput(PHASOR_LIGHT, PHASOR_OUTPUT);
    setLightFromOutput(PULSES_LIGHT, PULSES_OUTPUT);
    setBipolarLightBrightness(JITTER_POS_LIGHT, outputs[JITTER_OUTPUT].getVoltage() * 0.2f);
    setLightSmoothFromOutput(FINISH_LIGHT, FINISH_OUTPUT);
}


struct PhasorGenWidget : HCVModuleWidget { PhasorGenWidget(PhasorGen *module); };

PhasorGenWidget::PhasorGenWidget(PhasorGen *module)
{
	setSkinPath("res/PhasorGen.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 60.0f;
    const float knobX = 70.0f;
    const float spacing = 50.0f;

    createParamComboHorizontal(knobX, knobY,                PhasorGen::PHASE_PARAM, PhasorGen::PHASE_SCALE_PARAM, PhasorGen::PHASE_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing,      PhasorGen::PW_PARAM, PhasorGen::PW_SCALE_PARAM, PhasorGen::PW_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*2.0,  PhasorGen::PULSES_PARAM, PhasorGen::PULSES_SCALE_PARAM, PhasorGen::PULSES_INPUT);
    createParamComboHorizontal(knobX, knobY + spacing*3.0,  PhasorGen::JITTER_PARAM, PhasorGen::JITTER_SCALE_PARAM, PhasorGen::JITTER_INPUT);

    const float sRateX = 15.0f;
    const float sRateY = 60.0f;
    createParamComboVertical(sRateX, sRateY, PhasorGen::FREQ_PARAM, PhasorGen::FREQ_SCALE_PARAM, PhasorGen::FM_INPUT);


    createHCVSwitchVert(25.0f, 215.0f, PhasorGen::RANGE_PARAM);

    
	//////INPUTS//////
    const float jackY = 265.0f;
    createInputPort(22.0f, jackY, PhasorGen::VOCT_INPUT);
    createInputPort(78.0f, jackY, PhasorGen::CLOCK_INPUT);
    createInputPort(134.0f, jackY, PhasorGen::RESET_INPUT);
    createInputPort(190.0f, jackY, PhasorGen::FREEZE_INPUT);

	//////OUTPUTS//////
    const float outJackY = 315.0f;
    createOutputPort(22.0f, outJackY, PhasorGen::PHASOR_OUTPUT);
    createOutputPort(78.0f, outJackY, PhasorGen::PULSES_OUTPUT);
    createOutputPort(134.0f, outJackY, PhasorGen::JITTER_OUTPUT);
    createOutputPort(190.0f, outJackY, PhasorGen::FINISH_OUTPUT);

    createHCVRedLightForJack(22.0f, outJackY, PhasorGen::PHASOR_LIGHT);
    createHCVRedLightForJack(78.0f, outJackY, PhasorGen::PULSES_LIGHT);
    createHCVRedLightForJack(190.0f, outJackY, PhasorGen::FINISH_LIGHT);

    createHCVBipolarLightForJack(134.0f, outJackY, PhasorGen::JITTER_POS_LIGHT);
}

Model *modelPhasorGen = createModel<PhasorGen, PhasorGenWidget>("PhasorGen");
