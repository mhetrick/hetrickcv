
#include "HCVChaos.h"
#include "math.hpp"

void HCVLogisticMap::generate()
{
    lastValue = chaosAmount * lastValue * (1.0f - lastValue);
    lastValue = rack::math::clamp(lastValue, lowLimit, upperLimit);

    out1 = (lastValue - 0.6) * 1.6;
    out2 = out1 * -1.0f;
}

void HCVIkedaMap::generate()
{
    auto TN = 0.4f - 6.0f/(lastX + lastY + 1.0f);
    auto cosTN = cos(TN);
    auto sinTN = sin(TN);

    auto nextX = ((lastX * cosTN) - (lastY * sinTN)) * chaosAmount + 1.0f;
    auto nextY = ((lastY * cosTN) + (lastX * sinTN)) * chaosAmount;

    lastX = nextX;
    lastY = nextY;

    out1 = lastX * 0.3f;
    out2 = lastY * 0.3f;
}

void HCVTentMap::generate()
{
    out = chaosAmount * std::min(out, 1.0f-out);
    out1 = (out - 0.5f) * 2.0f;
    out2 = out1*-1.0f;
}

void HCVStandardMap::generate()
{
    float nextP = fmod((lastP + (chaosAmount * sin(lastO))), TWO_PI);
    float next0 = fmod((lastO + nextP), TWO_PI);

    lastP = nextP;
    lastO = next0;

    out1 = scaleOutput(lastP);
    out2 = scaleOutput(lastO);
}