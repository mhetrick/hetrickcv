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

struct XYToPolar : HCVModule
{
	enum ParamIds
	{
		NUM_PARAMS
	};
	enum InputIds
	{
        INX_INPUT,
        INY_INPUT,
        INR_INPUT,
        INTHETA_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUTR_OUTPUT,
        OUTTHETA_OUTPUT,
        OUTX_OUTPUT,
        OUTY_OUTPUT,
		NUM_OUTPUTS
    };

    enum LightIds
    {
        NUM_LIGHTS
	};

	XYToPolar()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void XYToPolar::process(const ProcessArgs &args)
{
    auto x = inputs[INX_INPUT].getVoltage();
    auto y = inputs[INY_INPUT].getVoltage();

    double r = sqrt(x*x + y*y);
    double theta;
    
    if(x == 0.0 && y == 0.0) theta = 0.0;
    else theta = atan2(y, x);

    outputs[OUTR_OUTPUT].setVoltage(r);
    outputs[OUTTHETA_OUTPUT].setVoltage(theta);

    if(inputs[INR_INPUT].isConnected()) r = inputs[INR_INPUT].getVoltage();
    if(inputs[INTHETA_INPUT].isConnected()) theta = inputs[INTHETA_INPUT].getVoltage();

    x = r * cos(theta);
    y = r * sin(theta);

    outputs[OUTX_OUTPUT].setVoltage(x);
    outputs[OUTY_OUTPUT].setVoltage(y);
}

struct XYToPolarWidget : HCVModuleWidget { XYToPolarWidget(XYToPolar *module); };

XYToPolarWidget::XYToPolarWidget(XYToPolar *module)
{
    setSkinPath("res/XYToPolar.svg");
    initializeWidget(module, true);

    //////PARAMS//////

    //////INPUTS//////
    const float jackXLeft = 9;
    const float jackXRight = 57;

    const int inRow1 = 62;
    const int inRow2 = 199;

    createInputPort(jackXLeft, inRow1, XYToPolar::INX_INPUT);
    createInputPort(jackXRight, inRow1, XYToPolar::INY_INPUT);
    createInputPort(jackXLeft, inRow2, XYToPolar::INR_INPUT);
    createInputPort(jackXRight, inRow2, XYToPolar::INTHETA_INPUT);

    const int outRow1 = 116;
    const int outRow2 = 251;
    createOutputPort(jackXLeft, outRow1, XYToPolar::OUTR_OUTPUT);
    createOutputPort(jackXRight, outRow1, XYToPolar::OUTTHETA_OUTPUT);
    createOutputPort(jackXLeft, outRow2, XYToPolar::OUTX_OUTPUT);
    createOutputPort(jackXRight, outRow2, XYToPolar::OUTY_OUTPUT);
}

Model *modelXYToPolar = createModel<XYToPolar, XYToPolarWidget>("XYToPolar");
