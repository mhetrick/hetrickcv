#pragma once

#include "HCVPhasor.h"
#include "dsp/common.hpp"
#include "HCVFunctions.h"
#include "HCVRandom.h"


static float scaleAndWrapPhasor(float _input)
{
    return gam::scl::wrap(_input * HCV_PHZ_DOWNSCALE);
}

class HCVPhasorSlopeDetector
{
public:
    float operator()(float _normalizedPhasorIn)
    {
        float slope = _normalizedPhasorIn - lastSample;
        lastSample = _normalizedPhasorIn;
        return gam::scl::wrap(_normalizedPhasorIn, 0.5f, -0.5f);
    }

private:
    float lastSample = 0.0f;
};

class HCVPhasorStepDetector
{
public:

    bool operator()(float _normalizedPhasorIn)
    {
        float scaledPhasor = _normalizedPhasorIn * numberSteps;
        int incomingStep = floorf(scaledPhasor);
        fractionalStep = scaledPhasor - incomingStep;

        if(incomingStep != currentStep)
        {
            currentStep = incomingStep;
            return true;
        }

        return false;
    }

    int getCurrentStep(){return currentStep;}
    void setNumberSteps(int _numSteps){numberSteps = std::max(1, _numSteps);}
    float getFractionalStep(){return fractionalStep;}

private:
    int currentStep = 0;
    int numberSteps = 1;
    float fractionalStep = 0.0f;
};

class HCVPhasorDivMult
{
public:

    float operator()(float _normalizedPhasorIn)
    {
        const float inSlope = slope(_normalizedPhasorIn);
        const float speedScale = multiplier/divider;
        const float scaledSlope = inSlope * speedScale;

        const float output = gam::scl::wrap(lastPhase + scaledSlope);
        lastPhase = output;
        return output;
    }

    float setMultiplier(float _multiplier){multiplier = _multiplier;}
    float setDivider(float _divider){divider = std::max(0.0001f, _divider);} //maybe clamp upper limit to prevent denormals?

    void reset(){lastPhase = resetPhase;}
    void enableAutosync(bool _autoSync){autoSync = _autoSync;}

protected:
    HCVPhasorSlopeDetector slope;
    float resetPhase = 0.0f;
    float lastPhase = 0.0f;
    float multiplier = 1.0f;
    float divider = 1.0f;

    bool autoSync = false;
};


class HCVPhasorToEuclidean
{
public:

    void processPhasor(float _inputPhasor)
    {
        float scaledRotation = quantizeRotation ? floorf(rotation * steps)/steps : rotation;
        const float scaledRamp = gam::scl::wrap(_inputPhasor*HCV_PHZ_DOWNSCALE + scaledRotation) * steps;
        const float stepWidth = scaledRamp - floorf(scaledRamp);
        clockOutput = stepWidth < pulseWidth ? gateScale : 0.0f;

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
        euclidGateOutput = phasorOutput < pulseWidth ? gateScale : 0.0f;

        
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
    const float outputScale = HCV_PHZ_UPSCALE;
    const float gateScale = 5.0f;
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

        if (_phasorIn < kinkPoint)
        {
            return gam::scl::mapLin(_phasorIn, 0.0f, kinkPoint, 0.0f, 0.5f);
        }

        return gam::scl::mapLin(_phasorIn, kinkPoint, 1.0f, 0.5f, 1.0f);
    }

    static float phasorKink(float _phasorIn, float _parameterIn)
    {
        float kinkPoint = (_parameterIn + 1.0f) * 0.5f;

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

class HCVPhasorRandomizer
{
public:
    float operator()(float _normalizedPhasor)
    {
        if(stepDetector(_normalizedPhasor))
        {
            randomizing = randomGen.nextProbability(probability);
            currentRandom = randomGen.nextFloat();

            if(mode == 0) offsetStep = randomGen.randomInt(currentNumSteps);
            else offsetStep = stepDetector.getCurrentStep();

            offsetBase = stepFraction * offsetStep;
        }

        gate = stepDetector.getFractionalStep() < 0.5f ? gateScale : 0.0f;

        if(randomizing)
        {
            steppedPhasor = offsetBase;

            switch (mode)
            {
            case 0: //random step
                return offsetBase + (stepDetector.getFractionalStep() * stepFraction);
            
            case 1: //random reverse slice
                return offsetBase + stepFraction - (stepDetector.getFractionalStep() * stepFraction);

            case 2: //random reverse phasor
                return (1.0 - offsetBase) - (stepDetector.getFractionalStep() * stepFraction);

            case 3: //random slope
                return offsetBase + 
                    gam::scl::clip(stepDetector.getFractionalStep() * stepFraction * currentRandom * 2.0f, stepFraction, 0.0f); 

            case 4: //random stutter
                return offsetBase + 
                    gam::scl::wrap(stepDetector.getFractionalStep() * stepFraction * int(1.0f + currentRandom * 7.0f), stepFraction, 0.0f); 

            case 5: //random freeze
                return offsetBase;

            default:
                return _normalizedPhasor;
            }

            
        }

        steppedPhasor = stepFraction * stepDetector.getCurrentStep();
        return _normalizedPhasor;
    }

    void setNumSteps(int _numSteps)
    {
        _numSteps = std::max(1, _numSteps);
        stepDetector.setNumberSteps(_numSteps);
        stepFraction = 1.0f/_numSteps;
        currentNumSteps = _numSteps;
    }

    void setProbability(float _prob)
    {
        probability = _prob;
    }

    float getSteppedPhasor()
    {
        return steppedPhasor;
    }

    float getGateOutput()
    {
        return gate;
    }

    void setMode(int _mode)
    {
        mode = _mode;
    }

protected:
    HCVPhasorStepDetector stepDetector;
    HCVRandom randomGen;
    bool randomizing = false;
    float stepFraction = 1.0f;
    float probability = 0.0f;
    float offsetBase = 0.0f;
    float steppedPhasor = 0.0f;
    float gate = 0.0f;
    float currentRandom = 0.0f;
    const float gateScale = 5.0f;
    int offsetStep = 0;
    int currentNumSteps = 1;
    int mode = 0;
};

class HCVPhasorJitter
{
public:

protected:

};