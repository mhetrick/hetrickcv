#include "HetrickCV.hpp"
#include "dsp/digital.hpp"

/*                         
 ▶─────┐            ┌─────▶
       │┌──────────┐│      
       ││  2 to 4  │├─────▶
       │└──────────┘│      
       ├─────▶──────┤      
       │            │      
       │            ├─────▶
       │            │      
 ▶─────┘            └─────▶
*/                         

struct TwoToFour : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        INA_INPUT,
        INB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
		NUM_OUTPUTS
	};

	enum LightIds
    {
        OUT1_POS_LIGHT, OUT1_NEG_LIGHT,
		OUT2_POS_LIGHT, OUT2_NEG_LIGHT,
		OUT3_POS_LIGHT, OUT3_NEG_LIGHT,
		OUT4_POS_LIGHT, OUT4_NEG_LIGHT,
        NUM_LIGHTS
	};

	TwoToFour()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

    simd::float_4   insA[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    insB[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outs1[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outs2[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outs3[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outs4[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void TwoToFour::process(const ProcessArgs &args)
{

    int channels = getMaxInputPolyphony();

	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c/4;
		insA[vectorIndex] = simd::float_4::load(inputs[INA_INPUT].getVoltages(c));
        insB[vectorIndex] = simd::float_4::load(inputs[INB_INPUT].getVoltages(c));
		outs1[vectorIndex] = insA[vectorIndex] + insB[vectorIndex];
        outs2[vectorIndex] = outs1[vectorIndex] * -1.0f;
        outs4[vectorIndex] = insA[vectorIndex] + insB[vectorIndex];
        outs3[vectorIndex] = outs4[vectorIndex] * -1.0f;
	}

	outputs[OUT1_OUTPUT].setChannels(channels);
    outputs[OUT2_OUTPUT].setChannels(channels);
    outputs[OUT3_OUTPUT].setChannels(channels);
    outputs[OUT4_OUTPUT].setChannels(channels);
	for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c/4;
		outs1[vectorIndex].store(outputs[OUT1_OUTPUT].getVoltages(c));
        outs2[vectorIndex].store(outputs[OUT2_OUTPUT].getVoltages(c));
        outs3[vectorIndex].store(outputs[OUT3_OUTPUT].getVoltages(c));
        outs4[vectorIndex].store(outputs[OUT4_OUTPUT].getVoltages(c));
	}

	lights[OUT1_POS_LIGHT].setSmoothBrightness(fmax(0.0f,  outs1[0][0] / 5.0f), args.sampleTime);
    lights[OUT1_NEG_LIGHT].setSmoothBrightness(fmax(0.0f, -outs1[0][0] / 5.0f), args.sampleTime);

    lights[OUT2_POS_LIGHT].setSmoothBrightness(fmax(0.0f,  outs2[0][0] / 5.0f), args.sampleTime);
    lights[OUT2_NEG_LIGHT].setSmoothBrightness(fmax(0.0f, -outs2[0][0] / 5.0f), args.sampleTime);

    lights[OUT3_POS_LIGHT].setSmoothBrightness(fmax(0.0f,  outs3[0][0] / 5.0f), args.sampleTime);
    lights[OUT3_NEG_LIGHT].setSmoothBrightness(fmax(0.0f, -outs3[0][0] / 5.0f), args.sampleTime);

    lights[OUT4_POS_LIGHT].setSmoothBrightness(fmax(0.0f,  outs4[0][0] / 5.0f), args.sampleTime);
    lights[OUT4_NEG_LIGHT].setSmoothBrightness(fmax(0.0f, -outs4[0][0] / 5.0f), args.sampleTime);
}

struct TwoToFourWidget : HCVModuleWidget { TwoToFourWidget(TwoToFour *module); };

TwoToFourWidget::TwoToFourWidget(TwoToFour *module)
{
	setSkinPath("res/2To4.svg");
    initializeWidget(module);

    //////PARAMS//////

    //////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(10, 100), module, TwoToFour::INA_INPUT));
    addInput(createInput<PJ301MPort>(Vec(55, 100), module, TwoToFour::INB_INPUT));

    for(int i = 0; i < 4; i++)
    {
        const int yPos = i*45;
        addOutput(createOutput<PJ301MPort>(Vec(33, 150 + yPos), module, TwoToFour::OUT1_OUTPUT + i));
        addChild(createLight<SmallLight<GreenRedLight>>(Vec(70, 158 + yPos), module, TwoToFour::OUT1_POS_LIGHT + i*2));
    }
}

Model *modelTwoToFour = createModel<TwoToFour, TwoToFourWidget>("2To4");

