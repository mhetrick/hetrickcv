#pragma once

#include "../HCVFunctions.h"
#include "HCVPhasorCommon.h"
#include "Gamma/Domain.h"
#include "Gamma/scl.h"
#include "dsp/digital.hpp"

class HCVPhasorSlopeDetector
{
public:
    float operator()(float _normalizedPhasorIn)
    {
        return calculateSteadySlope(_normalizedPhasorIn);
    }

    float calculateSteadySlope(float _normalizedPhasorIn)
    {
        slope = _normalizedPhasorIn - lastSample;
        lastSample = _normalizedPhasorIn;
        return gam::scl::wrap(slope, 0.5f, -0.5f);
    }

    float calculateRawSlope(float _normalizedPhasorIn)
    {
        slope = _normalizedPhasorIn - lastSample;
        lastSample = _normalizedPhasorIn;
        return slope;
    }

    float getSlopeInHz()
    {
        return slope * gam::Domain::master().spu(); //multiply slope by sample rate
    }

    float getSlopeInBPM()
    {
        return getSlopeInHz() * 60.0f;
    }

    float getSlopeDirection()
    {
        if(slope > 0.0f) return 1.0f;
        if(slope < 0.0f) return -1.0f;
        return 0.0f;
    }

    float getSlope() {return slope;}
    bool isPhasorAdvancing() { return slope != 0.0f;}

private:
    float lastSample = 0.0f;
    float slope = 0.0f;
};

class HCVPhasorResetDetector
{
public:
    bool operator()(float _normalizedPhasorIn)
    {
        return detectProportionalReset(_normalizedPhasorIn);
    }

    bool detectProportionalReset(float _normalizedPhasorIn);

    bool detectSimpleReset(float _normalizedPhasorIn)
    {
        return std::abs(slopeDetector.calculateRawSlope(_normalizedPhasorIn)) >= threshold;
    }

    void setThreshold(float _threshold)
    {
        threshold = clamp(_threshold, 0.0f, 1.0f);
    }

private:
    float lastSample = 0.0f;
    float threshold = 0.5f;
    HCVPhasorSlopeDetector slopeDetector;
    rack::dsp::BooleanTrigger repeatFilter;
};

class HCVPhasorStepDetector
{
public:

    bool operator()(float _normalizedPhasorIn);

    int getCurrentStep(){return currentStep;}
    void setNumberSteps(int _numSteps){numberSteps = std::max(1, _numSteps);}
    float getFractionalStep(){return fractionalStep;}

private:
    int currentStep = 0;
    int numberSteps = 1;
    float fractionalStep = 0.0f;
    HCVPhasorResetDetector resetDetector;
};