#include "HetrickCV.hpp"
#include "DSP/HCVCrackle.h"

/*                             
    ┌────────┐    crackle              
    │        │ ┌◖◖◖◖◖◖◖◖◖◖◖◖┐  
    │        │ │            │  
    │ ┌◅◅◅◅◅◅◅◅│◅◅◅◅┐       │  
  ──┼─┼───┬───┬─┬───┼─▶     │  
    │ │▫▫▫│▫▫▫│▫│▫▫▫│─.     │  
 ┌──┼─┴───┼──┐│▫│▫▫▫│  `.   │  
 │◓◓│◓◓◓◓◓│◓◓││▫├───┴────╳──┼─┐
 │◓◓│◓◓◓┌─┼─┬┼┼─┼─┐◘◘◘◘◘◘◘◘◘│◘│
 │◓◓├───┼─┘◓│││▫│◘│◘◘◘◘◘▲◘◘◘│◘│
 │◓◓│◓◓◓│◓◓◓│├┼─┤◘│◘◘◘◘◘│◘◘◘│◘│
 │◓◓│◓◓◓◓◓◓◓│││ │◘│◘◘◘◘◘└───┘◘│
 │◓◓│◓◓◓◓◓◓◓│││ │◘│◘◘◘◘◘◘◘◘◘◘◘│
 └──┼─╳─────┼┘│ │◘│◘◘◘◘◘◘◘◘◘◘◘│
  ▲ │  `.   ▴ │ ├─┼───────────┘
  │ │  ───────┘ │ │,─' ▲       
  └─┘       ▴───┼─│    │       
            └───┼─┴────┴──▶    
                │              
                └─────────────▶
*/                             

struct Crackle : HCVModule
{
    enum ParamIds
    {
        RATE_PARAM,
        BROKEN_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        RATE_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        MAIN_OUTPUT,
        NUM_OUTPUTS
    };

    Crackle()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
        configParam(Crackle::RATE_PARAM, 0.0, 2.0, 1.7, "Chaos Depth");
        configSwitch(Crackle::BROKEN_PARAM, 0.0, 1.0, 1.0, "Mode", {"Broken", "Normal"});

		configInput(RATE_INPUT, "Chaos CV");
		configOutput(MAIN_OUTPUT, "Crackle");

        resetCrackles();
    }
	
	void resetCrackles()
	{
		for (int c = 0; c < 16; c++)
		{
			crackle[c].reset();
		}
	}

	// Override the process method to handle audio processing

	void process(const ProcessArgs &args) override;

    void onReset() override
    {
        resetCrackles();
    }

    // Arrays for polyphonic support
    float lastDensity[16] = {};
    HCVCrackle crackle[16];

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};

void Crackle::process(const ProcessArgs &args)
{
    // Determine the number of channels based on connected inputs
    int channels = setupPolyphonyForAllOutputs();

    // Global mode setting (front-panel switch)
    const bool brokenMode = (params[BROKEN_PARAM].getValue() == 0.0);

    // Process each channel
    for (int c = 0; c < channels; c++)
    {
        const float densityInput = params[RATE_PARAM].getValue() + inputs[RATE_INPUT].getPolyVoltage(c);

        if(lastDensity[c] != densityInput)
        {
            float densityNormalized = clamp(densityInput, 0.0f, 2.0f) / 2.0f;
            crackle[c].setDensity(densityNormalized);
            lastDensity[c] = densityInput;
        }

        crackle[c].setBrokenMode(brokenMode);
        float output = crackle[c].generate();

        outputs[MAIN_OUTPUT].setVoltage(clamp(output * 5.0f, -5.0f, 5.0f), c);
    }
}


struct CrackleWidget : HCVModuleWidget { CrackleWidget(Crackle *module); };

CrackleWidget::CrackleWidget(Crackle *module)
{
	setSkinPath("res/Crackle.svg");
	initializeWidget(module);

    //////PARAMS//////
	createHCVKnob(28, 87, Crackle::RATE_PARAM);
    addParam(createParam<CKSS>(Vec(37, 220), module, Crackle::BROKEN_PARAM));

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(33, 146), module, Crackle::RATE_INPUT));

    //////OUTPUTS//////
	addOutput(createOutput<PJ301MPort>(Vec(33, 285), module, Crackle::MAIN_OUTPUT));
}

Model *modelCrackle = createModel<Crackle, CrackleWidget>("Crackle");
