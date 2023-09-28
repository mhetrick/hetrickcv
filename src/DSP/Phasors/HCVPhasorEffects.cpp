#include "HCVPhasorEffects.h"

//////////DIVMULT

float HCVPhasorDivMult::basicSync(float _normalizedPhasorIn)
{
    const float inSlope = slope(_normalizedPhasorIn);
    const double speedScale = double(multiplier)/double(divider);
    const double scaledSlope = double(inSlope) * speedScale;

    if(waitingToSync)
    {
        reset(_normalizedPhasorIn);
    }

    const double output = gam::scl::wrap(lastPhase + scaledSlope);
    lastPhase = output;
    return output;
}

float HCVPhasorDivMult::modulatedSync(float _normalizedPhasorIn)
{
    const float inSlope = slope(_normalizedPhasorIn);
    const double speedScale = multiplier/divider;
    const double scaledSlope = inSlope * speedScale;

    const float speedScaleDifference = std::abs((lastSpeedScale - speedScale)/(lastSpeedScale + speedScale));
    if(speedScaleDifference > threshold) waitingToSync = true;
    lastSpeedScale = speedScale;

    const double scaledPhase = _normalizedPhasorIn * speedScale;
    const double nextScaledPhase = lastPhase + scaledSlope;

    const float scaledPhaseDiff = nextScaledPhase - scaledPhase;
    const float roundedPhaseDiff = roundTruncMultiple(scaledPhaseDiff, speedScale); //TODO: Round to nearest multiple of speedScale

    bool inputReset = resetDetector.detectSimpleReset(_normalizedPhasorIn);

    double resetPhase = trunc(roundedPhaseDiff) + scaledPhase; 

    double selectedPhase;
    if(inputReset && waitingToSync)
    {
        selectedPhase = resetPhase;
        waitingToSync = false;
    } 
    else selectedPhase = nextScaledPhase;

    const double output = gam::scl::wrap(selectedPhase);
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
    if(resetDetector.detectProportionalReset(_normalizedPhasor))
    {
        swingGroup = std::max(1.0f, pendingGrouping);
        divider = 1.0f/swingGroup;
    }

    float slowPhasor = _normalizedPhasor * divider;
    float stepPhasor = slowPhasor * numSteps;
    float currentStep = floor(stepPhasor);
    float fractionalPhasor = stepPhasor - currentStep;
    float offsetBase = stepFraction * currentStep;

    if(groupDetector(slowPhasor))
    {
        swing = pendingSwing;
        variation = pendingVariation;

        totalSwing = swing + (variation * randomSource.whiteNoise());
        totalSwing = clamp(totalSwing, -swingScale, swingScale);
    }
    
    //choose different algorithms to change the flavor of the swing
    float swungPhasor = HCVPhasorEffects::phasorKink(fractionalPhasor, totalSwing);
    //float swungPhasor = HCVPhasorEffects::phasorCurve(fractionalPhasor, -totalSwing);
    //float swungPhasor = HCVPhasorEffects::phasorSplit(fractionalPhasor, -totalSwing);
    
    stepPhasorOutput = gam::scl::wrap(swungPhasor * swingGroup);

    return (offsetBase + swungPhasor * stepFraction) * swingGroup;
}

//// humanizer
void HCVPhasorHumanizer::generateNewValues()
{
    float sum = 0.0f;

    for (int i = 0; i < numSteps; i++)
    {
        float newValue = randomGen.whiteNoise() * 0.9f;
        sum += newValue;
        randomValues[i] = newValue;
    }

    const float average = sum/numSteps;
    
    //center the values around 1.0
    for (int i = 0; i < numSteps; i++)
    {
        randomValues[i] = (randomValues[i] - average) + 1.0f;
    }
}

float HCVPhasorHumanizer::operator()(float _normalizedPhasor)
{
    if (resetDetector(_normalizedPhasor))
    {
        lastPhase = 0.0f;
        if(!locked)
        {
            numSteps = pendingNumSteps;
            generateNewValues();
        }
    }

    if (numSteps == 1) return _normalizedPhasor;
    
    int currentStep = (int)floor(_normalizedPhasor * numSteps);
    float multiplier = randomValues[currentStep];

    const float scaledSlope = slope(_normalizedPhasor) * multiplier;

    //in rare cases, the humanized phasor will try to reset before the main phasor, so we clamp it to 1.0 instead of wrapping
    float humanizedPhasor = clamp(lastPhase + scaledSlope, 0.0f, 1.0f);
    lastPhase = humanizedPhasor;

    return LERP(depth, humanizedPhasor, _normalizedPhasor);
}

float HCVVariableBoundsPhasor::operator()(float _normalizedPhasor)
{
    lastPhase = gam::scl::wrap(lastPhase + slope(_normalizedPhasor), highBound, lowBound);

    return lastPhase;
}