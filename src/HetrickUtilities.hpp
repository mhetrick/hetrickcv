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

struct CKSSRot : SvgSwitch 
{
	CKSSRot() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_rot_1.svg")));
	}
};

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

    inline float getModulatedValue(int mainParamIndex, int cvInputIndex, int cvScaleIndex)
    {
        return params[mainParamIndex].getValue() + (inputs[cvInputIndex].getVoltage() * params[cvScaleIndex].getValue());
    }

    inline float getNormalizedModulatedValue(int mainParamIndex, int cvInputIndex, int cvScaleIndex)
    {
        return normalizeParameter(getModulatedValue(mainParamIndex, cvInputIndex, cvScaleIndex));
    }

    inline float getBipolarNormalizedModulatedValue(int mainParamIndex, int cvInputIndex, int cvScaleIndex)
    {
        return normalizeParameterBipolar(getModulatedValue(mainParamIndex, cvInputIndex, cvScaleIndex));
    }

    float getSampleRateParameter(int mainSRateIndex, int sRateCVIndex, int cvScaleIndex, int rangeIndex)
    {
        float sr = params[mainSRateIndex].getValue() + (inputs[sRateCVIndex].getVoltage() * params[cvScaleIndex].getValue() * 0.2f);
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

    static constexpr float HCV_GATE_MAG = 10.0f;
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
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	    if(!_is4HP) addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	    if(!_is4HP) addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }

	void initializeWidget(Module* module, bool _is4HP = false)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, skinPath)));
        createScrews(_is4HP);
    }

	void createHCVKnob(float _x, float _y, int _paramID)
    {
        //addParam(createParam<Davies1900hBlackKnob>(Vec(_x, _y), module, _paramID));
        addParam(createParam<Rogan1PRed>(Vec(_x + 2.5, _y), module, _paramID));
    }

    void createHCVTrimpot(float _x, float _y, int _paramID)
    {
        addParam(createParam<Trimpot>(Vec(_x, _y), module, _paramID));
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

    void createHCVButtonLarge(float _x, float _y, int _paramID)
    {
        addParam(createParam<CKD6>(Vec(_x, _y), module, _paramID));
    }

    void createHCVRedLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<RedLight>>(Vec(_x, _y), module, _lightID));
    }

    void createHCVGreenLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<GreenLight>>(Vec(_x, _y), module, _lightID));
    }

    void createHCVBipolarLight(float _x, float _y, int _lightID)
    {
        addChild(createLight<SmallLight<GreenRedLight>>(Vec(_x, _y), module, _lightID));
    }

    void createInputPort(float _x, float _y, int _paramID)
    {
        addInput(createInput<PJ301MPort>(Vec(_x, _y), module, _paramID));
    }

    void createOutputPort(float _x, float _y, int _paramID)
    {
        addOutput(createOutput<PJ301MPort>(Vec(_x, _y), module, _paramID));
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

/*
struct HCVTriggerGenerator
{
    bool triggered = false;

    bool process() 
    {
		if (triggered)
        {
            triggered = false;
            return true;
        }
        return false;
	}

    void trigger() 
    {
		triggered = true;
	}

    void reset()
    {
        triggered = false;
    }
};

struct TriggerGenWithSchmitt
{
	HCVTriggerGenerator trigGen;
	rack::dsp::SchmittTrigger schmitt;

	bool process(bool _trigger)
	{
		if(schmitt.process(_trigger ? 2.0f : 0.0f)) trigGen.trigger();
		return trigGen.process();
	}

    bool process(float _comparator)
    {
        if(schmitt.process(_comparator)) trigGen.trigger();
		return trigGen.process();
    }

    void reset()
    {
        trigGen.reset();
        schmitt.reset();
    }
};
*/

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
