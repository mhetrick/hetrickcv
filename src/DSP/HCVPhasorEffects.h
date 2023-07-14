#pragma once

#include "HCVPhasor.h"
#include "dsp/common.hpp"
#include "HCVFunctions.h"


static float scaleAndWrapPhasor(float _input)
{
    return gam::scl::wrap(_input * 0.2f);
}


class HCVPhasorToEuclidean
{
public:

    void processPhasor(float _inputPhasor)
    {
        float scaledRotation = quantizeRotation ? floorf(rotation * steps)/steps : rotation;
        const float scaledRamp = gam::scl::wrap(_inputPhasor*0.2f + scaledRotation) * steps;
        const float stepWidth = scaledRamp - floorf(scaledRamp);
        clockOutput = stepWidth < pulseWidth ? outputScale : 0.0f;

        if(fill == 0.0f)
        {
            phasorOutput = 0.0f;
            euclidGateOutput = 0.0f;
            return;
        }

        const float currentStep = floorf(scaledRamp);
        const float fillRatio = fill/steps;

        const float previousEvent = floorf(currentStep * fillRatio);
        const float nextEvent = ceilf((previousEvent + 1.0f)/fillRatio);
        const float currentEvent = ceilf(previousEvent/fillRatio);

        const float lengthBeats = nextEvent - currentEvent;

        phasorOutput = ((scaledRamp - currentEvent)/lengthBeats);
        euclidGateOutput = phasorOutput < pulseWidth ? outputScale : 0.0f;

        
    }

    void setBeats(float _beats)
    {
        steps = std::max(1.0f, _beats);
    }

    void setFill(float _fill)
    {
        fill = std::max(0.0f, _fill);
    }

    void setRotation(float _rotate)
    {
        rotation = rack::math::clamp(_rotate, -1.0f, 1.0f);
    }

    void setPulseWidth(float _pulseWidth)
    {
        pulseWidth = rack::math::clamp(_pulseWidth, 0.0f, 1.0f);
    }

    float getPhasorOutput() { return phasorOutput * outputScale; }
    float getEuclideanGateOutput() { return euclidGateOutput; }
    float getClockOutput() { return clockOutput;}

protected:
    float pulseWidth = 0.5f;

    //these are traditionally ints, but we use floats for calculations
    float steps = 16; //N
    float fill = 4; //K
    float rotation = 0; //S

    bool quantizeRotation = true;

    float phasorOutput = 0.0f;
    float euclidGateOutput = 0.0f;
    float clockOutput = 0.0f;
    const float outputScale = 5.0f;
};

//static effects
//parameters expected in [-1, 1] range
//phasors expected in [0, 1] range
//outputs [0, 1] range
class HCVPhasorEffects
{
public:
    static float phasorCurve(float _phasorIn, float _parameterIn)
    {
        float exponent;
        if (_parameterIn < 0.0f) exponent = 1.0f - (_parameterIn * -0.5f);
        else exponent = _parameterIn*2.0f + 1.0f;

        return powf(_phasorIn, exponent);
    }

    static float phasorShift(float _phasorIn, float _parameterIn)
    {
        return gam::scl::wrap(_phasorIn + _parameterIn);
    }

    static float phasorPinch(float _phasorIn, float _parameterIn)
    {
        float warpedPhasor;
        if(_parameterIn > 0.0f)
        {
            warpedPhasor = probit(_phasorIn);
            warpedPhasor = LERP(_parameterIn, warpedPhasor, _phasorIn);
        }
        else
        {
            warpedPhasor = sigmoidPhasor(_phasorIn);
            warpedPhasor = LERP(_parameterIn * -1.0f, warpedPhasor, _phasorIn);
        }

        return clamp(warpedPhasor, 0.0f, 1.0f);
    }

    static float phasorSplit(float _phasorIn, float _parameterIn)
    {
        float kinkPoint = (_parameterIn + 1.0f) * 0.5f;
		kinkPoint = clamp(kinkPoint, 0.001f, 0.99f);

        if (_phasorIn < kinkPoint)
        {
            return gam::scl::mapLin(_phasorIn, 0.0f, kinkPoint, 0.0f, 0.5f);
        }

        return gam::scl::mapLin(_phasorIn, kinkPoint, 1.0f, 0.5f, 1.0f);
    }

    static float phasorKink(float _phasorIn, float _parameterIn)
    {
        float kinkPoint = (_parameterIn + 1.0f) * 0.5f;
		kinkPoint = gam::scl::mapLin(kinkPoint, 0.0f, 1.0f, 0.05f, 0.95f);

        if (_phasorIn < 0.5f)
        {
            return gam::scl::mapLin(_phasorIn, 0.0f, 0.5f, 0.0f, kinkPoint);
        }
        
        return gam::scl::mapLin(_phasorIn, 0.5f, 1.0f, kinkPoint, 1.0f);
    }

    static float speedClip(float _phasorIn, float _parameterIn)
    {
        
        if(_parameterIn >= 0.0f)
        {
            const float mult = 1.0f + _parameterIn * 7.0f;
            return clamp(_phasorIn * mult, 0.0f, 1.0f);
        }

        const float mult = 1.0f - _parameterIn * 7.0f;
        return clamp(_phasorIn / mult, 0.0f, 1.0f);
    }

    static float speedWrap(float _phasorIn, float _parameterIn)
    {
        
        if(_parameterIn >= 0.0f)
        {
            const float mult = 1.0f + _parameterIn * 7.0f;
            return gam::scl::wrap(_phasorIn * mult);
        }

        const float mult = 1.0f - _parameterIn * 7.0f;
        return clamp(_phasorIn / mult, 0.0f, 1.0f); //no need to wrap
    }

    static float speedFold(float _phasorIn, float _parameterIn)
    {
        
        if(_parameterIn >= 0.0f)
        {
            const float mult = 1.0f + _parameterIn * 7.0f;
            return gam::scl::fold(_phasorIn * mult);
        }

        const float mult = 1.0f - _parameterIn * 7.0f;
        return clamp(_phasorIn / mult, 0.0f, 1.0f); //no need to fold
    }

private:

    static float myErfInv2(float x) 
    {
        const float sgn = (x < 0.0f) ? -1.0f : 1.0f;

        x = (1.0f - x) * (1.0f + x);        // x = 1 - x*x;
        const float lnx = logf(x);

        const float tt1 = 2.0f / (PI * 0.147f) + 0.5f * lnx;
        const float tt2 = 1.0f / (0.147f) * lnx;

        return(sgn * sqrtf(-tt1 + sqrtf(tt1 * tt1 - tt2)));
    }

    static float sigmoidPhasor(float x)
    {
        const float bipolarPhasor = x * 4.0f - 2.0f;
        const float bipolarOutput = erf(bipolarPhasor);
        return (bipolarOutput + 1.0f) * 0.5f;
    }

    static float probit(float x)
    {
        static float sqr2 = sqrtf(2.0f);
        x = clamp(x, 0.001f, 0.999f);
        const float unscaled = myErfInv2(2 * x - 1.0f);
        return (unscaled + 3.0f) * (1.0f/6.0f);
    }
};