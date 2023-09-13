#include "HetrickCV.hpp"
#include "DSP/HCVTiming.h"

/*                   
          ▲▲ delta   
         ▲▲▲▲        
        ▲▲▲▲▲▲       
       ▲▲▲▲▲▲▲▲      
      ▲▲▲▲▲▲▲▲▲▲     
     ▲▲▲▲▲▲▲▲▲▲▲▲    
    ▲▲▲▲▲▲▲▲▲▲▲▲▲▲   
   ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲  
  ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲ 
 ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
*/                   

struct Delta : HCVModule
{
	enum ParamIds
	{
		AMOUNT_PARAM,
        SCALE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        MAIN_INPUT,
        AMOUNT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		GT_GATE_OUTPUT,
		GT_TRIG_OUTPUT,
		LT_GATE_OUTPUT,
		LT_TRIG_OUTPUT,
		CHANGE_OUTPUT,
        DELTA_OUTPUT,
		NUM_OUTPUTS
	};

	 enum LightIds
    {
        GT_LIGHT,
        LT_LIGHT,
		CHANGE_LIGHT,
        NUM_LIGHTS
	};

	Delta()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Delta::AMOUNT_PARAM, 0.0, 5.0, 0.0, "Input Boost");
		configParam(Delta::SCALE_PARAM, -1.0, 1.0, 1.0, "Boost CV Depth");

		configInput(MAIN_INPUT, "Main");
		configInput(AMOUNT_INPUT, "Boost CV");

		configOutput(GT_GATE_OUTPUT, "Rising Gate");
		configOutput(GT_TRIG_OUTPUT, "Rising Trigger");
		configOutput(LT_GATE_OUTPUT, "Falling Gate");
		configOutput(LT_TRIG_OUTPUT, "Falling Trigger");
		configOutput(CHANGE_OUTPUT, "Direction Change Trigger");
		configOutput(DELTA_OUTPUT, "Delta");
	}

	HCVTriggeredGate ltTrig, gtTrig;
    float lastInput = 0.0f;
    bool rising = false;
    bool falling = false;

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Delta::process(const ProcessArgs &args)
{
	float input = inputs[MAIN_INPUT].getVoltage();

    float delta = input - lastInput;
    lastInput = input;

    rising = (delta > 0.0f);
    falling = (delta < 0.0f);

	float boost = params[AMOUNT_PARAM].getValue() + (inputs[AMOUNT_INPUT].getVoltage() * params[SCALE_PARAM].getValue());
	boost = clamp(boost, 0.0f, 5.0f) * 8000.0f + 1;

	outputs[GT_TRIG_OUTPUT].setVoltage(gtTrig.process(rising) ? HCV_GATE_MAG : 0.0f);
	outputs[LT_TRIG_OUTPUT].setVoltage(ltTrig.process(falling) ? HCV_GATE_MAG : 0.0f);
	outputs[GT_GATE_OUTPUT].setVoltage(rising  ? HCV_GATE_MAG : 0.0f);
	outputs[LT_GATE_OUTPUT].setVoltage(falling ? HCV_GATE_MAG : 0.0f);

	float allTrigs = outputs[GT_TRIG_OUTPUT].getVoltage() + outputs[LT_TRIG_OUTPUT].getVoltage();
	allTrigs = clamp(allTrigs, 0.0f, HCV_GATE_MAG);

    const float deltaOutput = clamp(delta * boost, -5.0f, 5.0f);

	outputs[CHANGE_OUTPUT].setVoltage(allTrigs);
    outputs[DELTA_OUTPUT].setVoltage(deltaOutput);

	setLightSmoothFromOutput(GT_LIGHT, GT_GATE_OUTPUT);
	setLightSmoothFromOutput(LT_LIGHT, LT_GATE_OUTPUT);
	lights[CHANGE_LIGHT].setSmoothBrightness(allTrigs, 10);
}


struct DeltaWidget : HCVModuleWidget { DeltaWidget(Delta *module); };

DeltaWidget::DeltaWidget(Delta *module)
{
	setSkinPath("res/Delta.svg");
	initializeWidget(module);

	//////PARAMS//////
	createHCVKnob(27, 62, Delta::AMOUNT_PARAM);
	createHCVTrimpot(36, 112, Delta::SCALE_PARAM);

	//////INPUTS//////
    addInput(createInput<PJ301MPort>(Vec(12, 195), module, Delta::MAIN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(33, 145), module, Delta::AMOUNT_INPUT));

	//////OUTPUTS//////
    addOutput(createOutput<PJ301MPort>(Vec(53, 195), module, Delta::DELTA_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(12, 285), module, Delta::LT_GATE_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(53, 285), module, Delta::GT_GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(12, 315), module, Delta::LT_TRIG_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(53, 315), module, Delta::GT_TRIG_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(32.5, 245), module, Delta::CHANGE_OUTPUT));

	//////BLINKENLIGHTS//////
	addChild(createLight<SmallLight<RedLight>>(Vec(22, 275), module, Delta::LT_LIGHT));
    addChild(createLight<SmallLight<GreenLight>>(Vec(62, 275), module, Delta::GT_LIGHT));
    addChild(createLight<SmallLight<RedLight>>(Vec(42, 275), module, Delta::CHANGE_LIGHT));
}

Model *modelDelta = createModel<Delta, DeltaWidget>("Delta");
