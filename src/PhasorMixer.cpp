#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorAnalyzers.h"

struct PhasorMixer : HCVModule
{
    static constexpr int NUM_MIX_CHANNELS = 5;

	enum ParamIds
	{
        ENUMS(LEVEL_PARAMS, NUM_MIX_CHANNELS),
		NUM_PARAMS
	};
	enum InputIds
	{
        ENUMS(PHASOR_INPUTS, NUM_MIX_CHANNELS),
		NUM_INPUTS
	};
	enum OutputIds
	{
        WRAP_OUTPUT,
        FOLD_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        WRAP_LIGHT,
        FOLD_LIGHT,
        NUM_LIGHTS
	};


	PhasorMixer()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < NUM_MIX_CHANNELS; i++)
        {
            configInput(PHASOR_INPUTS + i, "Phasor");
            configParam(LEVEL_PARAMS + i, 0.0f, 1.0f, 1.0f, "Phasor Gain");
        }
        
        configOutput(WRAP_LIGHT, "Wrapped Mix");
        configOutput(FOLD_LIGHT, "Folded Mix");
	}

	void process(const ProcessArgs &args) override;


	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorMixer::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    for (int i = 0; i < numChannels; i++)
    {
        float level = 0.0f;

        for (int j = 0; j < NUM_MIX_CHANNELS; j++)
        {
            level += (inputs[PHASOR_INPUTS + j].getPolyVoltage(i) * params[LEVEL_PARAMS + j].getValue());
        }
        
        outputs[WRAP_OUTPUT].setVoltage(gam::scl::wrap(level, 10.0f, 0.0f), i);
        outputs[FOLD_OUTPUT].setVoltage(gam::scl::fold(level, 10.0f, 0.0f), i);
    }
    
    setLightFromOutput(WRAP_LIGHT, WRAP_OUTPUT);
    setLightFromOutput(FOLD_LIGHT, FOLD_OUTPUT);
}

struct PhasorMixerWidget : HCVModuleWidget { PhasorMixerWidget(PhasorMixer *module); };

PhasorMixerWidget::PhasorMixerWidget(PhasorMixer *module)
{
    setSkinPath("res/PhasorMixer.svg");
    initializeWidget(module);

    for(int i = 0; i < PhasorMixer::NUM_MIX_CHANNELS; i++)
    {
        const int yPos = 62 + i*42;
        createInputPort(50, yPos, PhasorMixer::PHASOR_INPUTS + i);
        createHCVTrimpot(20, yPos + 3, PhasorMixer::LEVEL_PARAMS + i);
    }

    const int outY = 295.0f;

    createOutputPort(12, outY, PhasorMixer::WRAP_OUTPUT);
    createOutputPort(54, outY, PhasorMixer::FOLD_OUTPUT);

    createHCVRedLightForJack(10, outY, PhasorMixer::WRAP_LIGHT);
    createHCVRedLightForJack(56, outY, PhasorMixer::FOLD_LIGHT);

}

Model *modelPhasorMixer = createModel<PhasorMixer, PhasorMixerWidget>("PhasorMixer");
