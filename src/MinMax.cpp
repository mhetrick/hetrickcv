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

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void MinMax::process(const ProcessArgs &args)
{
    auto in1 = inputs[IN1_INPUT].getVoltage();
    auto in2 = inputs[IN2_INPUT].isConnected() ? inputs[IN2_INPUT].getVoltage() : in1;
    auto in3 = inputs[IN3_INPUT].isConnected() ? inputs[IN3_INPUT].getVoltage() : in2;
    auto in4 = inputs[IN4_INPUT].isConnected() ? inputs[IN4_INPUT].getVoltage() : in3;

    auto max1 = std::max(in1, in2);
    auto max2 = std::max(in3, in4);
    auto totalMax = std::max(max1, max2);

    auto min1 = std::min(in1, in2);
    auto min2 = std::min(in3, in4);
    auto totalMin = std::min(min1, min2);

    outputs[MIN_OUTPUT].setVoltage(totalMin);
    outputs[MAX_OUTPUT].setVoltage(totalMax);

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
