#include "HCVPhasorEffects.h"

//////////DIVMULT

float HCVPhasorDivMult::basicSync(float _normalizedPhasorIn)
{
    const float inSlope = slope(_normalizedPhasorIn);
    const float speedScale = multiplier/divider;
    const float scaledSlope = inSlope * speedScale;

    if(waitingToSync)
    {
        reset(_normalizedPhasorIn);
    }

    const float output = gam::scl::wrap(lastPhase + scaledSlope);
    lastPhase = output;
    return output;
}

float HCVPhasorDivMult::modulatedSync(float _normalizedPhasorIn)
{
    const float inSlope = slope(_normalizedPhasorIn);
    const float speedScale = multiplier/divider;
    const float scaledSlope = inSlope * speedScale;

    const float speedScaleDifference = std::abs((lastSpeedScale - speedScale)/(lastSpeedScale + speedScale));
    if(speedScaleDifference > threshold) waitingToSync = true;
    lastSpeedScale = speedScale;

    const float scaledPhase = _normalizedPhasorIn * speedScale;
    const float nextScaledPhase = lastPhase + scaledSlope;

    const float scaledPhaseDiff = nextScaledPhase - scaledPhase;
    const float roundedPhaseDiff = roundTruncMultiple(scaledPhaseDiff, speedScale); //TODO: Round to nearest multiple of speedScale

    bool inputReset = resetDetector.detectSimpleReset(_normalizedPhasorIn);

    float resetPhase = trunc(roundedPhaseDiff) + scaledPhase; 

    float selectedPhase;
    if(inputReset && waitingToSync)
    {
        selectedPhase = resetPhase;
        waitingToSync = false;
    } 
    else selectedPhase = nextScaledPhase;

    const float output = gam::scl::wrap(selectedPhase);
    lastPhase = output;
    return output;
}

float HCVPhasorDivMult::hardSynced(float _normalizedPhasorIn)
{
    if(resetDetector.detectSimpleReset(_normalizedPhasorIn)) reset();
    return basicSync(_normalizedPhasorIn);
}

////////EUCLIDEAN

void HCVPhasorToEuclidean::processPhasor(float _normalizedPhasor)
{
    float scaledRotation = quantizeRotation ? floorf(rotation * steps)/steps : rotation;
    const float scaledRamp = gam::scl::wrap(_normalizedPhasor + scaledRotation) * steps;
    const float stepWidth = scaledRamp - floorf(scaledRamp);
    clockOutput = clockGateDetector(stepWidth);

    const float currentStep = floorf(scaledRamp);
    if(stepDetector(_normalizedPhasor) || !quantizeParameterChanges)
    {
        lastStep = currentStep;
        steps = pendingSteps;
        fill = pendingFill;
        rotation = pendingRotation;
        stepDetector.setNumberSteps(steps);
    }

    if(fill == 0.0f)
    {
        phasorOutput = 0.0f;
        euclidGateOutput = 0.0f;
        return;
    }

    if(fill > steps)
    {

        const float fillRatio = fill/steps;
        const float ratchetLevel = ceilf(fill/steps);
        const float ratchetDepth = exp2(ratchetLevel - 1.0f);

        const float previousEvent = floorf(currentStep * fillRatio);
        const float nextEvent = ceilf((previousEvent + ratchetLevel)/fillRatio);
        const float currentEvent = ceilf(previousEvent/fillRatio);

        const float lengthBeats = nextEvent - currentEvent;

        phasorOutput = gam::scl::wrap((scaledRamp - currentEvent)/lengthBeats * ratchetDepth);
        euclidGateOutput = euclidGateDetector(phasorOutput);

        return;
    }

    const float fillRatio = fill/steps;

    const float previousEvent = floorf(currentStep * fillRatio);
    const float nextEvent = ceilf((previousEvent + 1.0f)/fillRatio);
    const float currentEvent = ceilf(previousEvent/fillRatio);

    const float lengthBeats = nextEvent - currentEvent;

    phasorOutput = ((scaledRamp - currentEvent)/lengthBeats);
    euclidGateOutput = euclidGateDetector(phasorOutput);
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

///// SWINGER

HCVPhasorSwingProcessor::HCVPhasorSwingProcessor()
{
    groupPhasor.setDivider(2.0f);
}

void HCVPhasorSwingProcessor::setNumStepsAndGrouping(float _numSteps, float _grouping)
{
    numSteps = std::max(1.0f, _numSteps);
    stepFraction = 1.0f/numSteps;

    pendingGrouping = _grouping;
    groupDetector.setNumberSteps(numSteps);
}

float HCVPhasorSwingProcessor::operator()(float _normalizedPhasor)
{
    if(resetDetector.detectSimpleReset(_normalizedPhasor))
    {
        groupPhasor.setDivider(pendingGrouping);
        swingGroup = std::max(1.0f, pendingGrouping);
    }

    float slowPhasor = groupPhasor.hardSynced(_normalizedPhasor);
    float stepPhasor = slowPhasor * numSteps;
    float currentStep = floor(stepPhasor);
    float fractionalPhasor = stepPhasor - currentStep;
    float offsetBase = stepFraction * currentStep;

    if(groupDetector(slowPhasor))
    {
        swing = pendingSwing;
        variation = pendingVariation;

        totalSwing = swing + (variation * randomSource.whiteNoise());
    }
    
    //choose different algorithms to change the flavor of the swing
    float swungPhasor = HCVPhasorEffects::phasorKink(fractionalPhasor, totalSwing);
    
    return (offsetBase + swungPhasor * stepFraction) * swingGroup;
}