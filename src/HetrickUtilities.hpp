#pragma once

#include "rack.hpp"
#include "engine/Engine.hpp"
#include "DSP/HCVFunctions.h"
#include "Gamma/Domain.h"

using namespace rack;
extern Plugin *pluginInstance;

/*                
         Λ        
        ╱░╲       
       ╱░░░Λ      
      ╱░░░╱▒╲     
     ╱░░░╱▒▒▒Λ    
    ╱░░░╱▒▒▒╱▓╲   
   ╱░░░╱▒▒▒╱▓▓▓Λ  
  ╱░░░╱▒▒▒╱▓▓▓╱█╲ 
 ▕░░░▕▒▒▒▕▓▓▓▕███▏
  ╲░░░╲▒▒▒╲▓▓▓╲█╱ 
   ╲░░░╲▒▒▒╲▓▓▓V  
    ╲░░░╲▒▒▒╲▓╱   
     ╲░░░╲▒▒▒V    
      ╲░░░╲▒╱     
       ╲░░░V      
        ╲░╱       
         V        
*/   

#define HCV_MAX_POLYPHONY 16

struct CKSSRot : SvgSwitch 
{
	CKSSRot() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_1.svg")));
	}
};

//convert +/-1.0 to frequency
float bipolarParamToOscillatorFrequencyScalar(float _bipolarParam);
float bipolarParamToLFOFrequencyScalar(float _bipolarParam);
float bipolarParamToClockMultScalar(float _bipolarParam);

//convert frequency to +/-1.0
float frequencyToBipolarParamUnscalar(float _frequency);
float lfoFrequencyToBipolarParamUnscalar(float _lfoFrequency);
float clockMultToBipolarParamUnscalar(float _clockMult);


struct HCVModule : Module
{
	float normalizeParameter(float value)
    {
        float temp = value*0.1+0.5;
        return clamp(temp, 0.0, 1.0);
    }

    float normalizeParameterBipolar(float value)
    {
        return clamp(value * 0.1f, -1.0f, 1.0f);
    }

    inline float boolToGate(bool _input)
    {
        return _input ? HCV_GATE_MAG : 0.0f;
    }

    inline float getModulatedValue(int mainParamIndex, int cvInputIndex, int cvScaleIndex, int channel = 0)
    {
        return params[mainParamIndex].getValue() + (inputs[cvInputIndex].getPolyVoltage(channel) * params[cvScaleIndex].getValue());
    }

    inline float getNormalizedModulatedValue(int mainParamIndex, int cvInputIndex, int cvScaleIndex, int channel = 0)
    {
        return normalizeParameter(getModulatedValue(mainParamIndex, cvInputIndex, cvScaleIndex, channel));
    }

    inline float getBipolarNormalizedModulatedValue(int mainParamIndex, int cvInputIndex, int cvScaleIndex, int channel = 0)
    {
        return normalizeParameterBipolar(getModulatedValue(mainParamIndex, cvInputIndex, cvScaleIndex, channel));
    }

    float getSampleRateParameter(int mainSRateIndex, int sRateCVIndex, int cvScaleIndex, int rangeIndex, int channel = 0)
    {
        float sr = params[mainSRateIndex].getValue() + (inputs[sRateCVIndex].getPolyVoltage(channel) * params[cvScaleIndex].getValue() * 0.2f);
        sr = clamp(sr, 0.01f, 1.0f);

        float finalSr = sr*sr*sr;
        finalSr = clamp(finalSr, 0.0f, 1.0f);

        if(params[rangeIndex].getValue() < 0.1f) finalSr = finalSr * 0.01f;

        return finalSr;
    }

    void onSampleRateChange() override
    {
        gam::sampleRate(APP->engine->getSampleRate());
    }

    int getMaxInputPolyphony()
    {
        int channels = 1;
        for (auto& input : inputs)
        {
            channels = std::max(channels, input.getChannels());
        }
        return channels;
    }

    int setupPolyphonyForAllOutputs()
    {
        int numChannels = getMaxInputPolyphony();
        for(auto& output : outputs)
        {
            output.setChannels(numChannels);
        }
        return numChannels;
    }

    void setBipolarLightBrightness(int _initialLightIndex, float _normalizedValue)
    {
        lights[_initialLightIndex].setBrightness(fmaxf(0.0f, _normalizedValue));
        lights[_initialLightIndex + 1].setBrightness(fmaxf(0.0f, _normalizedValue * -1.0f));
    }

    void setLightFromOutput(int _lightIndex, int _outputIndex, float _scale = 0.1f)
    {
        lights[_lightIndex].setBrightness(outputs[_outputIndex].getVoltage() * _scale);
    }
    void setLightSmoothFromOutput(int _lightIndex, int _outputIndex, float _scale = 0.1f)
    {
        lights[_lightIndex].setBrightnessSmooth(outputs[_outputIndex].getVoltage() * _scale, APP->engine->getSampleTime() * 4.0f);
    }

    static constexpr float HCV_GATE_MAG = 10.0f;
};

//many thanks to Marc at Impromptu for these excellent classes.
struct PanelBaseWidget : TransparentWidget {
	PanelBaseWidget(Vec _size) {
		box.size = _size;
	}
	void draw(const DrawArgs& args) override;
};

struct InverterWidget : TransparentWidget {
	// This method also has the main theme refresh stepper for the main panel's frame buffer.
	// It automatically makes DisplayBackground, SwitchOutlineWidget, etc. redraw when isDark() changes since they are children to the main panel's frame buffer   
	// Components such as ports and screws also have their own steppers (that just change the svg, since they don't have framebuffers)
	SvgPanel* mainPanel;
	int oldMode = -1;
	InverterWidget(SvgPanel* _mainPanel) {
		mainPanel = _mainPanel;
		box.size = mainPanel->box.size;
	}
	void refreshForTheme();
    void step() override;
	void draw(const DrawArgs& args) override;
};

struct HCVThemedRogan : rack::Rogan
{
    std::shared_ptr<window::Svg> lightBase = Svg::load(asset::system("res/ComponentLibrary/Rogan1PRed.svg"));
    std::shared_ptr<window::Svg> darkBase = Svg::load(asset::system("res/ComponentLibrary/Rogan1PBlue.svg"));
    std::shared_ptr<window::Svg> lightFG = Svg::load(asset::system("res/ComponentLibrary/Rogan1PRed_fg.svg"));
    std::shared_ptr<window::Svg> darkFG = Svg::load(asset::system("res/ComponentLibrary/Rogan1PBlue_fg.svg"));

    HCVThemedRogan()
    {
        bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1P_bg.svg")));
        setThemedSVGs();
    }

    void setThemedSVGs()
    {
        setSvg(settings::preferDarkPanels ? darkBase : lightBase);
        fg->setSvg(settings::preferDarkPanels ? darkFG : lightFG);
    }

    void step() override
    {
        setThemedSVGs();
        Rogan::step();
    }

};

template <typename BASE>
struct MuteLight : BASE {
	MuteLight() {
		this->box.size = mm2px(Vec(6.0, 6.0));
	}
};

struct HCVModuleWidget : ModuleWidget
{
	std::string skinPath = "";

	void setSkinPath(std::string _path)
    {
        skinPath = _path;
    }

	void createScrews(bool _is4HP = false)
    {
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
	    if(!_is4HP) addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	    if(!_is4HP) addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }

	void initializeWidget(Module* module, bool _is4HP = false)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, skinPath)));

        SvgPanel* svgPanel = (SvgPanel*)getPanel();
        //svgPanel->fb->addChildBottom(new PanelBaseWidget(svgPanel->box.size));
        svgPanel->fb->addChild(new InverterWidget(svgPanel));
        createScrews(_is4HP);
    }

	void createHCVKnob(float _x, float _y, int _paramID)
    {
        //addParam(createParam<Davies1900hBlackKnob>(Vec(_x, _y), module, _paramID));
        //addParam(createParam<Rogan1PRed>(Vec(_x + 2.5, _y), module, _paramID));
        addParam(createParam<HCVThemedRogan>(Vec(_x + 2.5, _y), module, _paramID));
    }

    void createHCVTrimpot(float _x, float _y, int _paramID)
    {
        addParam(createParam<Trimpot>(Vec(_x, _y), module, _paramID));
    }

    void createHCVTrimpotCentered(float _x, float _y, int _paramID)
    {
        addParam(createParamCentered<Trimpot>(Vec(_x, _y), module, _paramID));
    }

    void createHCVSwitchVert(float _x, float _y, int _paramID)
    {
        addParam(createParam<CKSS>(Vec(_x, _y), module, _paramID));
    }

    void createHCVSwitchHoriz(float _x, float _y, int _paramID)
    {
        addParam(createParam<CKSSRot>(Vec(_x, _y), module, _paramID));
    }

    void createHCVButtonSmall(float _x, float _y, int _paramID)
    {
        addParam(createParam<TL1105>(Vec(_x, _y), module, _paramID));
    }

    void createHCVButtonSmallForJack(float _x, float _y, int _paramID)
    {
        addParam(createParam<TL1105>(Vec(_x + 4, _y - 20), module, _paramID));
    }

    void createHCVButtonLarge(float _x, float _y, int _paramID)
    {
        addParam(createParam<CKD6>(Vec(_x, _y), module, _paramID));
    }

    void createHCVRedLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(_x, _y), module, _lightID));
    }
    void createHCVRedLightForJack(float _x, float _y, int _lightID)
    {
        createHCVRedLight(_x -5, _y - 2, _lightID);
    }

    void createHCVGreenLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<GreenLight>>(Vec(_x, _y), module, _lightID));
    }
    void createHCVGreenLightForJack(float _x, float _y, int _lightID)
    {
        createHCVGreenLight(_x - 5, _y - 2, _lightID);
    }

    void createHCVBipolarLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<GreenRedLight>>(Vec(_x, _y), module, _lightID));
    }
    void createHCVBipolarLightForJack(float _x, float _y, int _lightID)
    {
        createHCVBipolarLight(_x - 5, _y - 2, _lightID);
    }

    void createHCVTricolorLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(_x, _y), module, _lightID));
    }
    void createHCVTricolorLightForJack(float _x, float _y, int _lightID)
    {
        createHCVTricolorLight(_x - 5, _y - 2, _lightID);
    }


    void createInputPort(float _x, float _y, int _paramID)
    {
        addInput(createInput<ThemedPJ301MPort>(Vec(_x, _y), module, _paramID));
    }

    void createOutputPort(float _x, float _y, int _paramID)
    {
        addOutput(createOutput<ThemedPJ301MPort>(Vec(_x, _y), module, _paramID));
    }

    void createParamComboVertical(float _x, float _y, int _paramID, int _trimpotID, int _inputID)
    {
        const float knobY = _y;
        const float trimpotY = knobY + 58.0f;
        const float paramJackY = knobY + 108.0f;

        const float knobX = _x;
        const float trimpotX = knobX + 9.0f;
        const float paramJackX = knobX + 6.0f;

        createHCVKnob(knobX, knobY, _paramID);
	    createHCVTrimpot(trimpotX, trimpotY, _trimpotID);
        createInputPort(paramJackX, paramJackY, _inputID);
    }

    void createParamComboHorizontal(float _x, float _y, int _paramID, int _trimpotID, int _inputID)
    {
        const float knobY = _y;
        const float trimpotY = knobY + 4.0f;
        const float paramJackY = knobY + 3.0f;

        const float knobX = _x;
        const float trimpotX = knobX + 70.0f;
        const float paramJackX = knobX + 130.0f;

        createHCVKnob(knobX, knobY, _paramID);
        createHCVTrimpot(trimpotX, trimpotY, _trimpotID);
        createInputPort(paramJackX, paramJackY, _inputID);
    }

};

struct HysteresisGate
{
	bool state = false;
	float trueBound = 1.0f;
	float falseBound = 0.98f;

	bool process(float input)
	{
		if(input > trueBound) state = true;
		else if(input < falseBound) state = false;
		return state;
	}
	bool getState() {return state;}
};
