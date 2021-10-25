#include "HetrickCV.hpp"

struct Scanner : HCVModule
{
	enum ParamIds
	{
        SCAN_PARAM,
        STAGES_PARAM,
        WIDTH_PARAM,
        SLOPE_PARAM,
        OFFSET_PARAM,
        MIXSCALE_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        IN7_INPUT,
        IN8_INPUT,

        SCAN_INPUT,
        STAGES_INPUT,
        WIDTH_INPUT,
        SLOPE_INPUT,
        ALLIN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
        MIX_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
	{
        IN1_LIGHT,
        IN2_LIGHT,
        IN3_LIGHT,
        IN4_LIGHT,
        IN5_LIGHT,
        IN6_LIGHT,
        IN7_LIGHT,
        IN8_LIGHT,

        OUT1_POS_LIGHT, OUT1_NEG_LIGHT,
        OUT2_POS_LIGHT, OUT2_NEG_LIGHT,
        OUT3_POS_LIGHT, OUT3_NEG_LIGHT,
        OUT4_POS_LIGHT, OUT4_NEG_LIGHT,
        OUT5_POS_LIGHT, OUT5_NEG_LIGHT,
        OUT6_POS_LIGHT, OUT6_NEG_LIGHT,
        OUT7_POS_LIGHT, OUT7_NEG_LIGHT,
        OUT8_POS_LIGHT, OUT8_NEG_LIGHT,

		NUM_LIGHTS
	};

    float ins[8] = {};
    float outs[8] = {};
    float inMults[8] = {};
    float widthTable[9] = {0, 0, 0, 0.285, 0.285, 0.2608, 0.23523, 0.2125, 0.193};

	Scanner()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Scanner::SCAN_PARAM, 0, 5.0, 0.0, "Scan");
        configParam(Scanner::STAGES_PARAM, 0, 6.0, 6.0, "Number of Stages");
        configParam(Scanner::WIDTH_PARAM, 0, 5.0, 0.0, "Width");
        configParam(Scanner::SLOPE_PARAM, 0, 5.0, 0.0, "Slope");
        configSwitch(Scanner::OFFSET_PARAM, 0.0, 1.0, 0.0, "Voltage Offset", {"None", "+5V"});
        configParam(Scanner::MIXSCALE_PARAM, 0.0, 1.0, 0.125, "Mix Scale");
	}

    void process(const ProcessArgs &args) override;

    int clampInt(const int _in, const int min = 0, const int max = 7)
    {
        if (_in > max) return max;
        if (_in < min) return min;
        return _in;
    }

    float triShape(float _in)
    {
        _in = _in - round(_in);
        return std::abs(_in + _in);
    }

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void Scanner::process(const ProcessArgs &args)
{
    float allInValue = 0.0f;
    if(inputs[ALLIN_INPUT].isConnected()) allInValue = inputs[ALLIN_INPUT].getVoltage();
    else if(params[OFFSET_PARAM].getValue() != 0.0f) allInValue = 5.0f;

    for(int i = 0; i < 8; i++)
    {
        if(!inputs[IN1_INPUT + i].isConnected()) ins[i] = allInValue;
        else ins[i] = inputs[IN1_INPUT + i].getVoltage();
    }

    int stages = round(params[STAGES_PARAM].getValue() + inputs[STAGES_INPUT].getVoltage());
    stages = clampInt(stages, 0, 6) + 2;
    const float invStages = 1.0f/stages;
    const float halfStages = stages * 0.5f;
    const float remainInvStages = 1.0f - invStages;

    float widthControl = params[WIDTH_PARAM].getValue() + inputs[WIDTH_INPUT].getVoltage();
    widthControl = clamp(widthControl, 0.0f, 5.0f) * 0.2f;
    widthControl = widthControl * widthControl * widthTable[stages];

    float scanControl = params[SCAN_PARAM].getValue() + inputs[SCAN_INPUT].getVoltage();
    scanControl = clamp(scanControl, 0.0f, 5.0f) * 0.2f;

    float slopeControl = params[SLOPE_PARAM].getValue() + inputs[SLOPE_INPUT].getVoltage();
    slopeControl = clamp(slopeControl, 0.0f, 5.0f) * 0.2f;

    float scanFactor1 = LERP(widthControl, halfStages, invStages);
    float scanFactor2 = LERP(widthControl, halfStages + remainInvStages, 1.0f);
    float scanFinal = LERP(scanControl, scanFactor2, scanFactor1);

    float invWidth = 1.0f/(LERP(widthControl, float(stages), invStages+invStages));

    float subStage = 0.0f;
    for(int i = 0; i < 8; i++)
    {
        inMults[i] = (scanFinal + subStage) * invWidth;
        subStage = subStage - invStages;
    }

    for(int i = 0; i < 8; i++)
    {
        inMults[i] = clamp(inMults[i], 0.0f, 1.0f);
        inMults[i] = triShape(inMults[i]);
        inMults[i] = clamp(inMults[i], 0.0f, 1.0f);

        const float shaped = (2.0f - inMults[i]) * inMults[i];
        inMults[i] = LERP(slopeControl, shaped, inMults[i]);
    }

    outputs[MIX_OUTPUT].setVoltage(0.0f);

    for(int i = 0; i < 8; i++)
    {
        outputs[i].setVoltage(ins[i] * inMults[i]);

        lights[IN1_LIGHT + i].setSmoothBrightness(fmaxf(0.0, inMults[i]), 10);

        lights[OUT1_POS_LIGHT + 2*i].setSmoothBrightness(fmaxf(0.0, outputs[i].value / 5.0), 10);
        lights[OUT1_NEG_LIGHT + 2*i].setSmoothBrightness(fmaxf(0.0, outputs[i].value / -5.0), 10);
        outputs[MIX_OUTPUT].setVoltage(outputs[MIX_OUTPUT].value + outputs[i].value);
    }

    outputs[MIX_OUTPUT].setVoltage(outputs[MIX_OUTPUT].value * params[MIXSCALE_PARAM].getValue());
}


struct ScannerWidget : HCVModuleWidget { ScannerWidget(Scanner *module); };

ScannerWidget::ScannerWidget(Scanner *module)
{
    setSkinPath("res/Scanner.svg");
    initializeWidget(module);

    const int knobX = 75;
    const int jackX = 123;

    //////PARAMS//////
    createHCVKnob(knobX, 65, Scanner::SCAN_PARAM);
    createInputPort(jackX, 70, Scanner::SCAN_INPUT);

    createHCVKnob(knobX, 125, Scanner::STAGES_PARAM);
    createInputPort(jackX, 130, Scanner::STAGES_INPUT);

    createHCVKnob(knobX, 185, Scanner::WIDTH_PARAM);
    createInputPort(jackX, 190, Scanner::WIDTH_INPUT);

    createHCVKnob(knobX, 245, Scanner::SLOPE_PARAM);
    createInputPort(jackX, 250, Scanner::SLOPE_INPUT);

    createInputPort(96, 310, Scanner::ALLIN_INPUT);
    createOutputPort(141, 310, Scanner::MIX_OUTPUT);

    createHCVSwitchVert(75, 312, Scanner::OFFSET_PARAM);

    createHCVTrimpot(180, 313, Scanner::MIXSCALE_PARAM);

    const int inXPos = 10;
    const int inLightX = 50;

    const int outXPos = 235;
    const int outLightX = 210;

    for(int i = 0; i < 8; i++)
    {
        const int yPos = 50 + (40 * i);
        const int lightY = 59 + (40 * i);

        //////INPUTS//////
        addInput(createInput<PJ301MPort>(Vec(inXPos, yPos), module, i));

        //////OUTPUTS//////
        addOutput(createOutput<PJ301MPort>(Vec(outXPos, yPos), module, i));

        //////BLINKENLIGHTS//////
        addChild(createLight<SmallLight<RedLight>>(Vec(inLightX, lightY), module, Scanner::IN1_LIGHT + i));
        addChild(createLight<SmallLight<GreenRedLight>>(Vec(outLightX, lightY), module, Scanner::OUT1_POS_LIGHT + 2*i));
    }
}

Model *modelScanner = createModel<Scanner, ScannerWidget>("Scanner");
