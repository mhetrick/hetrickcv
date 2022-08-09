#include "HetrickCV.hpp"

/*            
      ╱       
     ╱        
    ╱       min  
   ╱        max  
  ╱           
 ▕      ╲     
  ╲      ╲    
   ╲      ╲   
    ╲      ╲  
     ╲      ╲ 
      ╲      ╲
             ╱
            ╱ 
           ╱  
          ╱   
         ╱    
        ╱     
*/            

struct MinMax : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        MIN_OUTPUT,
        MAX_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        MIN_POS_LIGHT,
        MIN_NEG_LIGHT,
		MAX_POS_LIGHT,
        MAX_NEG_LIGHT,
        NUM_LIGHTS
	};

	MinMax()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override;

    simd::float_4   ins1[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    ins2[4] = {0.0f, 0.0f, 0.0f, 0.0f},
                    ins3[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    ins4[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outsMax[4] = {0.0f, 0.0f, 0.0f, 0.0f}, 
                    outsMin[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void MinMax::process(const ProcessArgs &args)
{
    int channels = getMaxInputPolyphony();
    bool in2Connected = inputs[IN2_INPUT].isConnected();
    bool in3Connected = inputs[IN3_INPUT].isConnected();
    bool in4Connected = inputs[IN4_INPUT].isConnected();

    outputs[MIN_OUTPUT].setChannels(channels);
    outputs[MAX_OUTPUT].setChannels(channels);

    for (int c = 0; c < channels; c += 4) 
	{
        const int vectorIndex = c/4;
		ins1[vectorIndex] = simd::float_4::load(inputs[IN1_INPUT].getVoltages(c));
        ins2[vectorIndex] = in2Connected ? simd::float_4::load(inputs[IN2_INPUT].getVoltages(c)) : ins1[vectorIndex];
        ins3[vectorIndex] = in3Connected ? simd::float_4::load(inputs[IN3_INPUT].getVoltages(c)) : ins2[vectorIndex];
        ins4[vectorIndex] = in4Connected ? simd::float_4::load(inputs[IN4_INPUT].getVoltages(c)) : ins3[vectorIndex];

        auto max1 = fmax(ins1[vectorIndex], ins2[vectorIndex]);
        auto max2 = fmax(ins3[vectorIndex], ins4[vectorIndex]);
        auto totalMax = fmax(max1, max2);

        auto min1 = fmin(ins1[vectorIndex], ins2[vectorIndex]);
        auto min2 = fmin(ins3[vectorIndex], ins4[vectorIndex]);
        auto totalMin = fmin(min1, min2);

        outsMax[vectorIndex] = totalMax;
        outsMin[vectorIndex] = totalMin;

        outsMax[c / 4].store(outputs[MAX_OUTPUT].getVoltages(c));
        outsMin[c / 4].store(outputs[MIN_OUTPUT].getVoltages(c));
	}

    auto totalMin = outsMin[0][0];
    auto totalMax = outsMax[0][0];

    if (totalMin > 0.0)
    {
        lights[MIN_POS_LIGHT].setSmoothBrightness(totalMin * 0.2, args.sampleTime);
        lights[MIN_NEG_LIGHT].setSmoothBrightness(0.0, args.sampleTime);
    }
    else
    {
        lights[MIN_POS_LIGHT].setSmoothBrightness(0.0, args.sampleTime);
        lights[MIN_NEG_LIGHT].setSmoothBrightness(std::abs(totalMin) * 0.2, args.sampleTime);
    }

    if (totalMax > 0.0)
    {
        lights[MAX_POS_LIGHT].setSmoothBrightness(totalMax * 0.2, args.sampleTime);
        lights[MAX_NEG_LIGHT].setSmoothBrightness(0.0, args.sampleTime);
    }
    else
    {
        lights[MAX_POS_LIGHT].setSmoothBrightness(0.0, args.sampleTime);
        lights[MAX_NEG_LIGHT].setSmoothBrightness(std::abs(totalMax) * 0.2, args.sampleTime);
    }
    
}

struct MinMaxWidget : HCVModuleWidget { MinMaxWidget(MinMax *module); };

MinMaxWidget::MinMaxWidget(MinMax *module)
{
    setSkinPath("res/MinMax.svg");
    initializeWidget(module, true);

    //////PARAMS//////

    //////INPUTS//////
    const float inSpacing = 43.5f;
    const float jackX = 17.5f;

    for(int i = 0; i < MinMax::NUM_INPUTS; i++)
    {
        addInput(createInput<PJ301MPort>(Vec(jackX, 59 + (i*inSpacing)), module, MinMax::IN1_INPUT + i));
    }

    //////OUTPUTS//////
    const int outputY = 242;
    addOutput(createOutput<PJ301MPort>(Vec(jackX, outputY), module, MinMax::MAX_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(jackX, outputY + inSpacing), module, MinMax::MIN_OUTPUT));

    //////BLINKENLIGHTS//////
    int lightXLeft = 9;
    int lightXRight = 44;
    int maxY = 251;
    int minY = 294;
    addChild(createLight<SmallLight<RedLight>>(Vec(lightXLeft, maxY), module, MinMax::MAX_NEG_LIGHT));
    addChild(createLight<SmallLight<GreenLight>>(Vec(lightXRight, maxY), module, MinMax::MAX_POS_LIGHT));

    addChild(createLight<SmallLight<RedLight>>(Vec(lightXLeft, minY), module, MinMax::MIN_NEG_LIGHT));
    addChild(createLight<SmallLight<GreenLight>>(Vec(lightXRight, minY), module, MinMax::MIN_POS_LIGHT));
}

Model *modelMinMax = createModel<MinMax, MinMaxWidget>("MinMax");
