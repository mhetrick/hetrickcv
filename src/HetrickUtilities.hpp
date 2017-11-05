#pragma once

#include "dsp/digital.hpp"
#include "engine.hpp"

float LERP(const float _amount, const float _inA, const float _inB);

struct TriggerGenerator
{
    float time = 0.0;
	float triggerTime = 0.001;
    bool process() 
    {
		time += rack::engineGetSampleTime();
		return time < triggerTime;
	}
    void trigger() 
    {
		// Keep the previous pulseTime if the existing pulse would be held longer than the currently requested one.
        if (time + triggerTime >= triggerTime) 
        {
			time = 0.0;
		}
	}
};