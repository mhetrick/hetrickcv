#include "HetrickUtilities.hpp"

float bipolarParamToOscillatorFrequencyScalar(float _bipolarParam)
{
    const float scaledRange = _bipolarParam * 54.0f;
    const float semitone = powf(dsp::FREQ_SEMITONE, scaledRange);
    const float frequency = dsp::FREQ_C4 * semitone;

    return frequency;
}

float bipolarParamToLFOFrequencyScalar(float _bipolarParam)
{
    const float scaledRange = _bipolarParam * 9.0f + 1.0f;
    const float power = powf(2.0f, scaledRange);

    return power;
}

float bipolarParamToClockMultScalar(float _bipolarParam)
{
    return bipolarParamToLFOFrequencyScalar(_bipolarParam) * 0.5f;
}