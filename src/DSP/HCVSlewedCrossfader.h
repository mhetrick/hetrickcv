#pragma once

#include "dsp/filter.hpp"
#include "HCVSlewLimiter.h"

class HCVSlewedCrossfader
{
public:
    HCVSlewedCrossfader(float _defaultValue = 0.0)
    {
        slewValue = _defaultValue;
        slew.setRiseFall(100, 100);
    }

    float operator()(float _inOff, float _inOn)
    {
        return process(_inOff, _inOn);
    }

    float process(float _inOff, float _inOn)
    {
        float slewOut = slew.process(slewValue);
        return LERP(slewOut, _inOn, _inOff);
    }

    void setActive(bool _active)
    {
        slewValue = _active ? 1.0 : 0.0;
    }

    void setFader(float _fader)
    {
        slewValue = _fader;
    }

private:
    HCVSlewLimiter<> slew;
    float slewValue = 0.0;
};