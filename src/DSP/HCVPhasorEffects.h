#pragma once

#include "HCVPhasor.h"
#include "dsp/common.hpp"


float scaleAndWrapPhasor(float _input)
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
        const float currentStep = floorf(scaledRamp);
        const float fillRatio = fill/steps;

        const float previousEvent = floorf(currentStep * fillRatio);
        const float nextEvent = ceilf((previousEvent + 1.0f)/fillRatio);
        const float currentEvent = ceilf(previousEvent/fillRatio);

        const float lengthBeats = nextEvent - currentEvent;

        phasorOutput = ((scaledRamp - currentEvent)/lengthBeats);
        euclidGateOutput = phasorOutput < pulseWidth ? outputScale : 0.0f;

        const float stepWidth = scaledRamp - floorf(scaledRamp);
        clockOutput = stepWidth < pulseWidth ? outputScale : 0.0f;
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