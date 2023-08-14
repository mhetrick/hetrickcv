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

float frequencyToBipolarParamUnscalar(float _frequency)
{
    const float semitone = _frequency/dsp::FREQ_C4;
    const float scaledRange = std::log10(semitone)/std::log10(dsp::FREQ_SEMITONE);
    const float bipolar = scaledRange/54.0f;

    return clamp(bipolar, -1.0f, 1.0f);
}
float lfoFrequencyToBipolarParamUnscalar(float _lfoFrequency)
{
    const float root = std::log2(_lfoFrequency) - 1.0f;
    const float bipolar = root/9.0f;

    return clamp(bipolar, -1.0f, 1.0f);
}
float clockMultToBipolarParamUnscalar(float _clockMult)
{
    return lfoFrequencyToBipolarParamUnscalar(_clockMult * 2.0f);
}