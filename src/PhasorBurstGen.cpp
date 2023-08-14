#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h" 
#include "DSP/Phasors/HCVPhasor.h"
#include "dsp/approx.hpp"

struct PhasorBurstGen : HCVModule
{
	enum ParamIds
	{
		FREQ_PARAM, FREQ_SCALE_PARAM,

        PW_PARAM, PW_SCALE_PARAM,
        REPEATS_PARAM, REPEATS_SCALE_PARAM,

        RANGE_PARAM,
        PASS_PARAM,

        RESET_PARAM,
        STOP_PARAM,
        CYCLE_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
        FM_INPUT,
        PW_INPUT,
        REPEATS_INPUT,

        VOCT_INPUT,

        CLOCK_INPUT,
        RESET_INPUT,
        STOP_INPUT,
        FREEZE_INPUT,
        CYCLE_INPUT,
        
		NUM_INPUTS
	};
	enum OutputIds
	{
		PHASOR_OUTPUT,
        PULSES_OUTPUT,
        PASS_OUTPUT,
        FINISH_OUTPUT,
		NUM_OUTPUTS
	};
    enum LightIds
    {
        PHASOR_LIGHT,
        PULSES_LIGHT,
        PASS_LIGHT,
        FINISH_LIGHT,
        CYCLE_LIGHT,
        NUM_LIGHTS
	};

    const float MAX_REPEATS = 64.0f;
    const float REPEATS_CV_SCALAR = MAX_REPEATS/5.0f;

    dsp::SchmittTrigger cycleTrigger;
    dsp::SchmittTrigger stopTriggers[16];
    dsp::SchmittTrigger resetTriggers[16];
    HCVTriggeredGate passTriggers[16];
    bool cycling = false;

	PhasorBurstGen()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		struct FrequencyQuantity : ParamQuantity {
			float getDisplayValue() override {
				PhasorBurstGen* module = reinterpret_cast<PhasorBurstGen*>(this->module);
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

        configSwitch(PhasorBurstGen::RANGE_PARAM, 0.0, 1.0, 0.0, "Speed Range", {"Slow", "Fast"});

		configParam<FrequencyQuantity>(FREQ_PARAM, -1.f, 1.f, 0.f, "Frequency");
		configParam(PhasorBurstGen::FREQ_SCALE_PARAM, -1.0, 1.0, 1.0, "Cycle Frequency CV Depth");

        configParam(PhasorBurstGen::PW_PARAM, -5.0, 5.0, 0.0, "Pulse Width");
		configParam(PhasorBurstGen::PW_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulse Width CV Depth");

        configParam(PhasorBurstGen::REPEATS_PARAM, 1, MAX_REPEATS, 1, "Repeats");
        paramQuantities[REPEATS_PARAM]->snapEnabled = true;
		configParam(PhasorBurstGen::REPEATS_SCALE_PARAM, -1.0, 1.0, 1.0, "Repeats CV Depth");

        
        configSwitch(PhasorBurstGen::PASS_PARAM, 0.0, 1.0, 0.0, "Reset Behavior", {"Always Reset, even if bursting", "If bursting, send trigger to pass output instead"});

        configButton(RESET_PARAM, "Reset");
        configButton(STOP_PARAM, "Stop");
        configButton(CYCLE_PARAM, "Cycle Toggle");

        configInput(FM_INPUT, "Frequency CV");

        configInput(PW_INPUT, "Pulse Width CV");
        configInput(REPEATS_INPUT, "Repeats CV");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(STOP_INPUT, "Stop");
        configInput(FREEZE_INPUT, "Freeze");
        configInput(CYCLE_INPUT, "Cycle");

        configOutput(PHASOR_OUTPUT, "Phasor");
        configOutput(PULSES_OUTPUT, "Pulses");
        configOutput(PASS_OUTPUT, "Passed Trigger");
        configOutput(FINISH_OUTPUT, "Finished Trigger");
	}

	void process(const ProcessArgs &args) override;

    HCVBurstPhasor phasors[16];
    HCVClockSync clockSyncs[16];

    json_t *dataToJson() override
    {
		json_t *rootJ = json_object();
        json_object_set_new(rootJ, "cycling", json_boolean(cycling));
		return rootJ;
	}
    void dataFromJson(json_t *rootJ) override
    {
		json_t *cyclingJ = json_object_get(rootJ, "cycling");
		if (cyclingJ)
            cycling = json_boolean_value(cyclingJ);
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void PhasorBurstGen::process(const ProcessArgs &args)
{
    const float freqKnob = params[FREQ_PARAM].getValue();
    const float fmCVKnob = params[FREQ_SCALE_PARAM].getValue();

    const float repeatsKnob = params[REPEATS_PARAM].getValue();
    const float repeatsCVDepth = params[REPEATS_SCALE_PARAM].getValue();

    const float pwKnob = params[PW_PARAM].getValue();
    const float pwCVDepth = params[PW_SCALE_PARAM].getValue();

    const bool lfoMode = params[RANGE_PARAM].getValue() == 0.0f;
    const bool passMode = params[PASS_PARAM].getValue() > 0.0f;

    const float resetButton = params[RESET_PARAM].getValue();
    const float stopButton = params[STOP_PARAM].getValue();

    const bool cycleButton = cycleTrigger.process(params[CYCLE_PARAM].getValue());
    if(cycleButton) cycling = !cycling;

    int numChannels = setupPolyphonyForAllOutputs();

    for (int i = 0; i < numChannels; i++)
    {
        if(inputs[CLOCK_INPUT].isConnected()) //clock mode
        {
            clockSyncs[i].processGateClockInput(inputs[CLOCK_INPUT].getPolyVoltage(i));
            const float baseClockFreq = clockSyncs[i].getBaseClockFreq();

            float pitch =  freqKnob + inputs[VOCT_INPUT].getPolyVoltage(i);
            pitch += (inputs[FM_INPUT].getPolyVoltage(i) * fmCVKnob);

            float freq = baseClockFreq * rack::dsp::approxExp2_taylor5(pitch);

            phasors[i].setFreqDirect(freq);
        }
        else //freq mode
        {
            float pitchParamValue = freqKnob;
            if(!lfoMode) pitchParamValue = pitchParamValue * 4.5f;//54.0f / 12.0f;
            else pitchParamValue = pitchParamValue * 9.0f + 1.0f;

            float pitch = pitchParamValue + inputs[VOCT_INPUT].getPolyVoltage(i);
            pitch += (inputs[FM_INPUT].getPolyVoltage(i) * fmCVKnob);

            float baseFreq = lfoMode ? 1.0f : dsp::FREQ_C4;
            float freq = baseFreq * rack::dsp::approxExp2_taylor5(pitch);

            freq = clamp(freq, 0.f, args.sampleRate / 2.f);
            phasors[i].setFreqDirect(freq);
        }

        //cycle input acts as a momentary toggle for internal cycling state
        const bool cycleMode = (inputs[CYCLE_INPUT].getPolyVoltage(i) >= 1.0f) ? !cycling : cycling;

        float modulatedRepeats = repeatsCVDepth * inputs[REPEATS_INPUT].getPolyVoltage(i) * REPEATS_CV_SCALAR;
        float repeats = clamp(repeatsKnob + modulatedRepeats, 1.0f, MAX_REPEATS);
        phasors[i].setRepeats(cycleMode ? 1 : repeats);

        float pulseWidth = pwKnob + (pwCVDepth * inputs[PW_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;;
        phasors[i].setPulseWidth(pulseWidth);

        phasors[i].setFrozen(inputs[FREEZE_INPUT].getPolyVoltage(i) >= 1.0f);

        bool phasorJustStopped = false;
        const float stopValue = inputs[STOP_INPUT].getPolyVoltage(i) + stopButton;
        if(stopTriggers[i].process(stopValue))
        {
            cycling = false;
            phasors[i].stopPhasor();
            phasorJustStopped = true;
        } 
        
        bool finished = phasors[i].done();

        const bool reset = (inputs[RESET_INPUT].getPolyVoltage(i) + resetButton) >= 1.0f;
        bool triggerPass = false;
        if(resetTriggers[i].process(reset))
        {
            if(passMode)
            {
                if(finished) phasors[i].reset();
                else triggerPass = true;
            }
            else
            {
                phasors[i].reset();
            }
        }
        
        if(cycleMode && finished) phasors[i].reset();

        const float phasorOutput = phasorJustStopped ? 0.0f : phasors[i]();
        finished = phasors[i].done(); //check again for output muting. gamma's phasor stays high when finished.

        outputs[PHASOR_OUTPUT].setVoltage(finished ? 0.0f : phasorOutput, i);
        outputs[PULSES_OUTPUT].setVoltage(finished ? 0.0f : phasors[i].getPulse(), i); 
        outputs[PASS_OUTPUT].setVoltage(passTriggers[i].process(triggerPass) ? HCV_PHZ_GATESCALE : 0.0f, i);

        outputs[FINISH_OUTPUT].setVoltage( finished ? HCV_GATE_MAG : 0.0f, i);
    }

    setLightFromOutput(PHASOR_LIGHT, PHASOR_OUTPUT);
    setLightFromOutput(PULSES_LIGHT, PULSES_OUTPUT);
    setLightSmoothFromOutput(PASS_LIGHT, PASS_OUTPUT);
    setLightFromOutput(FINISH_LIGHT, FINISH_OUTPUT);

    const bool cycleMode = (inputs[CYCLE_INPUT].getVoltage() >= 1.0f) ? !cycling : cycling;
    lights[CYCLE_LIGHT].setBrightness(cycleMode ? 1.0f : 0.0f);
}


struct PhasorBurstGenWidget : HCVModuleWidget { PhasorBurstGenWidget(PhasorBurstGen *module); };

PhasorBurstGenWidget::PhasorBurstGenWidget(PhasorBurstGen *module)
{
	setSkinPath("res/PhasorBurstGen.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 60.0f;
    const float knobX = 15.0f;
    const float spacing = 87.0f;

    createParamComboVertical(knobX, knobY,              PhasorBurstGen::FREQ_PARAM, PhasorBurstGen::FREQ_SCALE_PARAM, PhasorBurstGen::FM_INPUT);
    createParamComboVertical(knobX + spacing, knobY,    PhasorBurstGen::REPEATS_PARAM, PhasorBurstGen::REPEATS_SCALE_PARAM, PhasorBurstGen::REPEATS_INPUT);
    createParamComboVertical(knobX + spacing*2.0, knobY,PhasorBurstGen::PW_PARAM, PhasorBurstGen::PW_SCALE_PARAM, PhasorBurstGen::PW_INPUT);

    createHCVSwitchVert(25.0f, 215.0f, PhasorBurstGen::RANGE_PARAM);
    createHCVSwitchVert(68.0f, 215.0f, PhasorBurstGen::PASS_PARAM);

    
	//////INPUTS//////
    const float jackY = 265.0f;
    createInputPort(22.0f, jackY, PhasorBurstGen::VOCT_INPUT);
    createInputPort(64.0f, jackY, PhasorBurstGen::CLOCK_INPUT);
    createInputPort(106.0f, jackY, PhasorBurstGen::RESET_INPUT);
    createInputPort(148.0f, jackY, PhasorBurstGen::STOP_INPUT);
    createInputPort(190.0f, jackY, PhasorBurstGen::CYCLE_INPUT);

    createHCVButtonSmallForJack(106.0f, jackY, PhasorBurstGen::RESET_PARAM);
    createHCVButtonSmallForJack(148.0f, jackY, PhasorBurstGen::STOP_PARAM);
    createHCVButtonSmallForJack(190.0f, jackY, PhasorBurstGen::CYCLE_PARAM);

    createHCVRedLightForJack(190.0f, jackY, PhasorBurstGen::CYCLE_LIGHT);

	//////OUTPUTS//////
    const float outJackY = 315.0f;
    createOutputPort(22.0f, outJackY, PhasorBurstGen::PHASOR_OUTPUT);
    createOutputPort(78.0f, outJackY, PhasorBurstGen::PULSES_OUTPUT);
    createOutputPort(134.0f, outJackY, PhasorBurstGen::PASS_OUTPUT);
    createOutputPort(190.0f, outJackY, PhasorBurstGen::FINISH_OUTPUT);

    createHCVRedLightForJack(22.0f, outJackY, PhasorBurstGen::PHASOR_LIGHT);
    createHCVRedLightForJack(78.0f, outJackY, PhasorBurstGen::PULSES_LIGHT);
    createHCVRedLightForJack(134.0f, outJackY, PhasorBurstGen::PASS_LIGHT);
    createHCVRedLightForJack(190.0f, outJackY, PhasorBurstGen::FINISH_LIGHT);
}

Model *modelPhasorBurstGen = createModel<PhasorBurstGen, PhasorBurstGenWidget>("PhasorBurstGen");
