#pragma once

#include "HCVPhasor.h"
#include "HCVPhasorAnalyzers.h"

#include "../HCVRandom.h"
#include "HCVPhasorCommon.h"

class HCVPhasorDivMult
{
public:

    float operator()(float _normalizedPhasorIn)
    {
        if(autoSync) return modulatedSync(_normalizedPhasorIn);
        else return basicSync(_normalizedPhasorIn);
    }

    float basicSync(float _normalizedPhasorIn);
    float modulatedSync(float _normalizedPhasorIn);
    float hardSynced(float _normalizedPhasorIn);

    void setMultiplier(const float _multiplier)
    {
        multiplier = std::max(0.0001f, _multiplier);
    }
    void setDivider(const float _divider)
    {
        divider = std::max(0.0001f, _divider);
    }
    void reset(float _resetPhase = 0.0f)
    {
        lastPhase = _resetPhase;
        waitingToSync = false;    
    }
    void resync()
    {
        waitingToSync = true;
    }

    void enableAutosync(bool _autoSync){autoSync = _autoSync;}

    static float roundTruncMultiple( float value, float multiple )
    {
        if (multiple == 0) return value;
        return std::trunc(value/multiple)*multiple;
    }

protected:
    HCVPhasorSlopeDetector slope;
    HCVPhasorResetDetector resetDetector;
    
    float multiplier = 1.0f;
    float divider = 1.0f;

    float threshold = 1.0f/64.0f;

    double lastPhase = 0.0;
    double lastSpeedScale = 1.0;

    bool autoSync = false;
    bool waitingToSync = false;
};


class HCVPhasorToEuclidean
{
public:

    HCVPhasorToEuclidean()
    {
        stepDetector.setNumberSteps(steps);
    }

    void processPhasor(float _normalizedPhasor);

    void setBeats(float _beats)
    {
        pendingSteps = std::max(1.0f, _beats);
    }

    void setFill(float _fill)
    {
        pendingFill = std::max(0.0f, _fill);
    }

    void setRotation(float _rotate)
    {
        pendingRotation = rack::math::clamp(_rotate, -1.0f, 1.0f);
    }

    void setPulseWidth(float _pulseWidth)
    {
        pulseWidth = rack::math::clamp(_pulseWidth, 0.0f, 1.0f);
        clockGateDetector.setGateWidth(pulseWidth);
        euclidGateDetector.setGateWidth(pulseWidth);
    }

    void enableSmartDetection(bool smartModeEnabled)
    {
        clockGateDetector.setSmartMode(smartModeEnabled);
        euclidGateDetector.setSmartMode(smartModeEnabled);
    }

    void setParameterChangeQuantization(bool quantizationEnabled)
    {
        quantizeParameterChanges = quantizationEnabled;
    }

    float getPhasorOutput() { return phasorOutput * outputScale; }
    float getEuclideanGateOutput() { return euclidGateOutput; }
    float getClockOutput() { return clockOutput;}

    void setRotationQuantization(bool _quantizationEnabled)
    {
        quantizeRotation = _quantizationEnabled;
    }

protected:
    float pulseWidth = 0.5f;

    //these are traditionally ints, but we use floats for calculations
    float steps = 16, pendingSteps = 16; //N
    float fill = 4, pendingFill = 4; //K
    float rotation = 0, pendingRotation = 0; //S

    float lastStep = 0.0f;

    bool quantizeRotation = true;
    bool quantizeParameterChanges = true;

    float phasorOutput = 0.0f;
    float euclidGateOutput = 0.0f;
    float clockOutput = 0.0f;
    const float outputScale = HCV_PHZ_UPSCALE;
    const float gateScale = HCV_PHZ_GATESCALE;
    HCVPhasorGateDetector euclidGateDetector;
    HCVPhasorGateDetector clockGateDetector;
    HCVPhasorStepDetector stepDetector;
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

    static float triangleShaper(float _phasorIn, float _parameterIn)
    {
        const float skew = clamp((_parameterIn + 1.0f) * 0.5f, 0.0001f, 0.9999f);
        const float s = 1.0f/skew;
        const float t = 1.0f/(1.0f - skew);

        const float trianglePhasor = std::min(s*_phasorIn, t * (1.0f - _phasorIn));
        return trianglePhasor;
    }

    static float arcShaper(float _phasorIn, float _parameterIn)
    {
        if(_parameterIn > 0.9999f) return 1.0f;
        const float skew = clamp((_parameterIn + 1.0f) * 0.5f, 0.0f, 0.9999f);

        const float curve = (2.0f/3.0f) * (2.0f * skew - skew*skew)/(1.0f - skew);

        return (_phasorIn * curve) / (1.0f + _phasorIn * (curve - 1.0f));
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
    float operator()(float _normalizedPhasor);

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

    float getRandomPhasor()
    {
        return (randomizing || forceRandomization) ? stepDetector.getFractionalStep() : 0.0f;
    }

    float getRandomGate()
    {
        return (randomizing || forceRandomization) ? gate : 0.0f;
    }

    float getGateOutput()
    {
        return gate;
    }

    void setMode(int _mode)
    {
        mode = _mode;
    }

    void enableForceRandomization(bool _forceRandomization)
    {
        forceRandomization = _forceRandomization;
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
    float randomPhasor = 0.0f;

    const float gateScale = HCV_PHZ_GATESCALE;
    int offsetStep = 0;
    int currentNumSteps = 1;
    int mode = 0;

    bool forceRandomization = false;
};

class HCVPhasorJitter
{
public:

protected:

};


//Designed by Graham Wakefield and Gregory Taylor
//Generating Sound and Organizing Time
//in Chapter 3: Unit Shaping

class HCVPhasorLFO
{
public:
    void setWidthParam(float _width)
    {
        width = 1.0f - clamp(_width, 0.0f, 1.0f);
    }
    void setTrapezoidParam(float _trapezoidShape)
    {
        trapezoidShape = clamp(_trapezoidShape, 0.0f, 1.0f);
    }
    void setPhaseParam(float _phase)
    {
        phase = clamp(_phase, -1.0f, 1.0f);
    }
    void setSinusoidParam(float _sinusoid)
    {
        sinusoid = clamp(_sinusoid, 0.0f, 1.0f);
    }

    float operator()(float _normalizedPhasor)
    {
        trianglePhasor = 1.0f - HCVPhasorEffects::triangleShaper(_normalizedPhasor, phase);

        const float offsetPhasor = trianglePhasor - width;

        if(trapezoidShape > 0.999f) return offsetPhasor > 0.0f ? 1.0f : 0.0f;

        const float steepness = 1.0f / (1.0f - trapezoidShape);
        const float trapezoidShaped = clamp(offsetPhasor * steepness + width, 0.0f, 1.0f);

        const float sineShaped = (1.0f - cosf(trapezoidShaped * M_PI)) * 0.5f;

        pulse = _normalizedPhasor < (1.0f - width) ? HCV_PHZ_GATESCALE : 0.0f;

        return LERP(sinusoid, sineShaped, trapezoidShaped);
    }

    float getTriangle()
    {
        return trianglePhasor;
    }

    float getPulse()
    {
        return pulse;
    }

protected:
    float width = 0.0f;
    float trapezoidShape = 0.0f;
    float phase = 0.0001f;
    float sinusoid = 0.0f;

    float trianglePhasor = 0.0f;
    float pulse = 0.0f;
};

class HCVPhasorSwingProcessor
{
public:
    HCVPhasorSwingProcessor();
    float operator()(float _normalizedPhasor);
    
    //expects a bipolar parameter range of [-1.0f, 1.0f]
    //Negative numbers make the upstep early, positive numbers make the upstep late
    void setSwing(float _swing)
    {
        pendingSwing = clamp(_swing * -swingScale, -swingScale, swingScale);
    }
    void setVariation(float _variation)
    {
        pendingVariation = clamp(_variation * _variation, 0.0f, 1.0f);
    }
    
    void setNumStepsAndGrouping(float _numSteps, float _grouping);

    float getStepPhasorOutput()
    {
        return stepPhasorOutput;
    }

protected:
    HCVPhasorResetDetector resetDetector;
    HCVPhasorStepDetector groupDetector;
    HCVRandom randomSource;

    static constexpr float swingScale = 0.95f;

    float stepPhasorOutput = 0.0f;
    float numSteps = 16.0f;
    float swingGroup = 2.0f;
    float stepFraction = 1.0f/16.0f;
    float pendingSwing = 0.0f;
    float pendingVariation = 0.0f;
    float variation = 0.0f;
    float swing = 0.0f;
    float totalSwing = 0.0f;
    float pendingGrouping = 2.0f;
    float divider = 0.5f;
};


//Thanks to Philip Meyer for the recipe from here: https://www.youtube.com/watch?v=ZD4_dDWA0sc
class HCVPhasorHumanizer
{
public:
    HCVPhasorHumanizer(int maxSteps = 64)
    {
        randomValues.resize(maxSteps);
        generateNewValues();
    }

    void setNumSteps(int _numSteps)
    {
        pendingNumSteps = std::max(1, _numSteps);
    }

    void setDepth(float _depth)
    {
        depth = _depth * _depth * _depth;
    }

    void reset(float _resetPhase)
    {
        lastPhase = _resetPhase;
    }

    void generateNewValues();
    float operator()(float _normalizedPhasor);

protected:
    std::vector<float> randomValues;
    int pendingNumSteps = 8;
    int numSteps = 8;

    float lastPhase = 0.0f;
    HCVRandom randomGen;
    HCVPhasorSlopeDetector slope;
    HCVPhasorResetDetector resetDetector;
    float depth = 0.1f;
    bool locked = false;
};

class HCVVariableBoundsPhasor
{
public:

    float operator()(float _normalizedPhasor);

    void setBounds(float _lowBound, float _highBound)
    {
        if(_lowBound > _highBound)
        {
            lowBound = _highBound;
            highBound = _lowBound;
        }
        else
        {
            lowBound = _lowBound;
            highBound = _highBound;
        }
    }

protected:
    float lowBound = 0.0f;
    float highBound = 1.0f;
    float lastPhase = 0.0f;

    HCVPhasorSlopeDetector slope;
};