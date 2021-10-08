
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

void HCVHenonMap::generate()
{
    float nextX = 1.0f - (lastX*lastX*chaosAmountA) + lastY;
    float nextY = lastX * chaosAmountB;

    lastX = rack::math::clamp(nextX, -1.0f, 1.0f);
    lastY = rack::math::clamp(nextY, -1.0f, 1.0f);

    out1 = lastX;
    out2 = lastY;
}

void HCVHetrickMap::generate()
{
    float nextY = rack::math::clamp(lastX2 * chaosAmountB, -1.0f, 1.0f);
    float nextX = rack::math::clamp(1.0f - (nextY + (lastX*lastX*chaosAmountA)), -1.0f, 1.0f);

    lastX2 = lastX;
    lastX = nextX;

    out1 = lastX;
    out2 = nextY;
}

void HCVCuspMap::generate()
{
    float nextX = chaosAmountA - (std::sqrt(std::abs(lastX)) * chaosAmountB);
    lastX = rack::math::clamp(nextX, -1.0f, 1.0f);

    out1 = lastX;
    out2 = lastX * -1.0f;
}

void HCVGaussMap::generate()
{
    float a = (lastX - chaosAmountA);
    float base = (a*a)/(chaosAmountB * chaosAmountB * -2.0f);
    float nextX = rack::math::clamp(std::exp(base), -1.0f, 1.0f);

    lastX = nextX;
    out1 = lastX;
    out2 = lastX * -1.0f;
}

void HCVMouseMap::generate()
{
    float base = (lastX * lastX * chaosAmountA * -1.0f);
    float nextX = std::exp(base) + chaosAmountB;
    nextX = rack::math::clamp(nextX, -1.0f, 1.0f);

    lastX = nextX;
    out1 = lastX;
    out2 = lastX * -1.0f;
}

void HCVLCCMap::generate()
{
    float base = (lastOut * chaosAmountA) + chaosAmountB;
    float unscaled = fmodf(base, chaosAmountC);
    lastOut = unscaled;

    float nextOut = ((2.0f/chaosAmountC) * unscaled) - 1.0f;
    out = rack::math::clamp(nextOut, -1.0f, 1.0f);
}

void HCVQuadraticMap::generate()
{
    float aFactor = lastOut*lastOut*chaosAmountA;
    float bFactor = (lastOut * chaosAmountB) + aFactor;
    float nextOut = rack::math::clamp(bFactor + chaosAmountC, -1.0f, 1.0f);
    lastOut = nextOut;
    out = uniToBi(lastOut);
}