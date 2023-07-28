#include "HCVPhasorAnalyzers.h"

bool HCVPhasorResetDetector::detectProportionalReset(float _normalizedPhasorIn)
{
    const float difference = _normalizedPhasorIn - lastSample;
    const float sum = _normalizedPhasorIn + lastSample;
    lastSample = _normalizedPhasorIn;
    if(sum == 0.0f) return false;

    const float proportionalChange = std::abs(difference/sum);

    const bool resetDetected = proportionalChange > threshold;

    //return resetDetected;
    return repeatFilter.process(resetDetected);
}

bool HCVPhasorStepDetector::operator()(float _normalizedPhasorIn)
{
    float scaledPhasor = _normalizedPhasorIn * numberSteps;
    int incomingStep = floorf(scaledPhasor);
    fractionalStep = scaledPhasor - incomingStep;

    if(numberSteps == 1)
    {
        currentStep = 0;
        stepChanged = resetDetector.detectSimpleReset(_normalizedPhasorIn);
        return stepChanged;
    }

    if(incomingStep != currentStep)
    {
        currentStep = incomingStep;
        stepChanged = true;
        return stepChanged;
    }

    stepChanged = false;
    return stepChanged;
}

float HCVPhasorGateDetector::getSmartGate(float normalizedPhasor)
{
    //only change reverse direction detection if phasor is moving, otherwise high frequency noise will result ;)
    const float slope = slopeDetector(normalizedPhasor);
    const bool phasorIsAdvancing = slopeDetector.isPhasorAdvancing();
    if(phasorIsAdvancing) reversePhasor = slope < 0.0f;
    bool isZero = normalizedPhasor == 0.0f;

    if(phasorIsAdvancing || !isZero)
    {
        float gate;
        if (reversePhasor) gate = (1.0f - normalizedPhasor) < gateWidth ? HCV_PHZ_GATESCALE : 0.0f;
        else gate = normalizedPhasor < gateWidth ? HCV_PHZ_GATESCALE : 0.0f;

        return gate;
    }
    
    return 0.0f;
}