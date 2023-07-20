#include "HCVPhasorAnalyzers.h"

bool HCVPhasorResetDetector::detectProportionalReset(float _normalizedPhasorIn)
{
    const float difference = _normalizedPhasorIn - lastSample;
    const float sum = _normalizedPhasorIn + lastSample;
    if(sum == 0.0f) return false;

    const float proportionalChange = std::abs(difference/sum);

    const bool resetDetected = proportionalChange > threshold;

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

//TODO
bool HCVPhasorStepDetector::detectStepTransitionSmart(float _normalizedPhasorIn)
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