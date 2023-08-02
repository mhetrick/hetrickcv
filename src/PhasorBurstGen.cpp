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
        NUM_LIGHTS
	};

    const float MAX_REPEATS = 64.0f;
    const float REPEATS_CV_SCALAR = MAX_REPEATS/5.0f;

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
                        minValue = -54.0f;
                        maxValue = 54.0f;
                        defaultValue = 0.0f;

                        displayBase = dsp::FREQ_SEMITONE;
                        displayMultiplier = dsp::FREQ_C4;
                    }
                    else //LFO
                    {
                        minValue = -8.0f;
                        maxValue = 10.0f;
                        defaultValue = 1.0f;

                        displayMultiplier = 1.f;
                        displayBase = 2.0f;
                    }
				}
				else //clock sync div/mult
                {
					unit = "x";

                    minValue = -8.0f;
                    maxValue = 10.0f;
                    defaultValue = 1.0f;

                    displayBase = 2.0f;
					displayMultiplier = 1 / 2.f;
				}
				return ParamQuantity::getDisplayValue();
			}
		};
		configParam<FrequencyQuantity>(FREQ_PARAM, -8.f, 10.f, 1.f, "Frequency", " Hz", 2, 1);
		configParam(PhasorBurstGen::FREQ_SCALE_PARAM, -1.0, 1.0, 1.0, "Cycle Frequency CV Depth");

        configParam(PhasorBurstGen::PW_PARAM, -5.0, 5.0, 0.0, "Pulse Width");
		configParam(PhasorBurstGen::PW_SCALE_PARAM, -1.0, 1.0, 1.0, "Pulse Width CV Depth");

        configParam(PhasorBurstGen::REPEATS_PARAM, 1, MAX_REPEATS, 1, "Repeats");
        paramQuantities[REPEATS_PARAM]->snapEnabled = true;
		configParam(PhasorBurstGen::REPEATS_SCALE_PARAM, -1.0, 1.0, 1.0, "Repeats CV Depth");

        configSwitch(PhasorBurstGen::RANGE_PARAM, 0.0, 1.0, 1.0, "Speed Range", {"Slow", "Fast"});
        configSwitch(PhasorBurstGen::PASS_PARAM, 0.0, 1.0, 0.0, "Reset Behavior", {"Always Reset, even if bursting", "If bursting, send trigger to pass output instead"});

        configButton(RESET_PARAM, "Reset");
        configButton(STOP_PARAM, "Stop");

        configInput(FM_INPUT, "Frequency CV");

        configInput(PW_INPUT, "Pulse Width CV");
        configInput(REPEATS_INPUT, "Repeats CV");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(STOP_INPUT, "Stop");
        configInput(FREEZE_INPUT, "Freeze");

        configOutput(PHASOR_OUTPUT, "Phasor");
        configOutput(PULSES_OUTPUT, "Pulses");
        configOutput(PASS_OUTPUT, "Passed Trigger");
        configOutput(FINISH_OUTPUT, "Finished Trigger");
	}

	void process(const ProcessArgs &args) override;

    HCVBurstPhasor phasors[16];
    HCVClockSync clockSyncs[16];

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

    int numChannels = setupPolyphonyForAllOutputs();

    for (int i = 0; i < numChannels; i++)
    {
        if(inputs[CLOCK_INPUT].isConnected()) //clock mode
        {
            clockSyncs[i].processGateClockInput(inputs[CLOCK_INPUT].getPolyVoltage(i));
            const float baseClockFreq = clockSyncs[i].getBaseClockFreq();

            float pitch =  freqKnob + inputs[VOCT_INPUT].getPolyVoltage(i);
            pitch += (inputs[FM_INPUT].getPolyVoltage(i) * fmCVKnob);

            float freq = baseClockFreq * 0.5f * rack::dsp::approxExp2_taylor5(pitch);

            phasors[i].setFreqDirect(freq);
        }
        else //freq mode
        {
            float pitchParamValue = freqKnob;
            if(!lfoMode) pitchParamValue = pitchParamValue / 12.0f;
            float pitch = pitchParamValue + inputs[VOCT_INPUT].getPolyVoltage(i);
            pitch += (inputs[FM_INPUT].getPolyVoltage(i) * fmCVKnob);

            float baseFreq = lfoMode ? 1.0f : dsp::FREQ_C4;
            float freq = baseFreq * rack::dsp::approxExp2_taylor5(pitch);

            freq = clamp(freq, 0.f, args.sampleRate / 2.f);
            phasors[i].setFreqDirect(freq);
        }

        float modulatedRepeats = repeatsCVDepth * inputs[REPEATS_INPUT].getPolyVoltage(i) * REPEATS_CV_SCALAR;
        float repeats = clamp(repeatsKnob + modulatedRepeats, 1.0f, MAX_REPEATS);
        phasors[i].setRepeats(repeats);

        float pulseWidth = pwKnob + (pwCVDepth * inputs[PW_INPUT].getPolyVoltage(i));
        pulseWidth = clamp(pulseWidth, -5.0f, 5.0f) * 0.1f + 0.5f;;
        phasors[i].setPulseWidth(pulseWidth);

        phasors[i].setFrozen(inputs[FREEZE_INPUT].getPolyVoltage(i) >= 1.0f);
        bool resetThisFrame = phasors[i].processGateResetInput(inputs[RESET_INPUT].getPolyVoltage(i));

        bool finished = phasors[i].done();

        outputs[PHASOR_OUTPUT].setVoltage(finished ? 0.0f : phasors[i](), i);
        outputs[PULSES_OUTPUT].setVoltage(finished ? 0.0f : phasors[i].getPulse(), i); 
        outputs[PASS_OUTPUT].setVoltage(0.0f, i);

        outputs[FINISH_OUTPUT].setVoltage( finished ? HCV_GATE_MAG : 0.0f, i);
    }

    setLightFromOutput(PHASOR_LIGHT, PHASOR_OUTPUT);
    setLightFromOutput(PULSES_LIGHT, PULSES_OUTPUT);
    setLightSmoothFromOutput(PASS_LIGHT, PASS_OUTPUT);
    setLightFromOutput(FINISH_LIGHT, FINISH_OUTPUT);
}


struct PhasorBurstGenWidget : HCVModuleWidget { PhasorBurstGenWidget(PhasorBurstGen *module); };

PhasorBurstGenWidget::PhasorBurstGenWidget(PhasorBurstGen *module)
{
	setSkinPath("res/PhasorBurstGen.svg");
	initializeWidget(module);

	//////PARAMS//////
    const float knobY = 60.0f;
    const float knobX = 15.0f;
    const float spacing = 88.0f;

    createParamComboVertical(knobX, knobY,              PhasorBurstGen::FREQ_PARAM, PhasorBurstGen::FREQ_SCALE_PARAM, PhasorBurstGen::FM_INPUT);
    createParamComboVertical(knobX + spacing, knobY,    PhasorBurstGen::REPEATS_PARAM, PhasorBurstGen::REPEATS_SCALE_PARAM, PhasorBurstGen::REPEATS_INPUT);
    createParamComboVertical(knobX + spacing*2.0, knobY,PhasorBurstGen::PW_PARAM, PhasorBurstGen::PW_SCALE_PARAM, PhasorBurstGen::PW_INPUT);

    createHCVSwitchVert(25.0f, 215.0f, PhasorBurstGen::RANGE_PARAM);
    createHCVSwitchVert(81.0f, 215.0f, PhasorBurstGen::PASS_PARAM);

    
	//////INPUTS//////
    const float jackY = 265.0f;
    createInputPort(22.0f, jackY, PhasorBurstGen::VOCT_INPUT);
    createInputPort(78.0f, jackY, PhasorBurstGen::CLOCK_INPUT);
    createInputPort(134.0f, jackY, PhasorBurstGen::RESET_INPUT);
    createInputPort(190.0f, jackY, PhasorBurstGen::STOP_INPUT);

    createHCVButtonSmallForJack(134.0f, jackY, PhasorBurstGen::RESET_PARAM);
    createHCVButtonSmallForJack(190.0f, jackY, PhasorBurstGen::STOP_PARAM);

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
