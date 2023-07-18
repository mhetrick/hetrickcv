#include "HCVPhasorEffects.h"

//////////DIVMULT

float HCVPhasorDivMult::basicSync(float _normalizedPhasorIn)
{
    const float inSlope = slope(_normalizedPhasorIn);
    const float speedScale = multiplier/divider;
    const float scaledSlope = inSlope * speedScale;

    const float output = gam::scl::wrap(lastPhase + scaledSlope);
    lastPhase = output;
    return output;
}

float HCVPhasorDivMult::modulatedSync(float _normalizedPhasorIn)
{
    const float inSlope = slope(_normalizedPhasorIn);
    const float speedScale = multiplier/divider;
    const float scaledSlope = inSlope * speedScale;

    const float output = gam::scl::wrap(lastPhase + scaledSlope);
    lastPhase = output;
    return output;
}

float HCVPhasorDivMult::hardSynced(float _normalizedPhasorIn)
{
    if(resetDetector.detectSimpleReset(_normalizedPhasorIn)) reset();
    return basicSync(_normalizedPhasorIn);
}

////////EUCLIDEAN

void HCVPhasorToEuclidean::processPhasor(float _inputPhasor)
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


/////////RANDOMIZER
float HCVPhasorRandomizer::operator()(float _normalizedPhasor)
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
    steppedPhasor = stepFraction * stepDetector.getCurrentStep();

    if(randomizing || forceRandomization)
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

    return _normalizedPhasor;
}