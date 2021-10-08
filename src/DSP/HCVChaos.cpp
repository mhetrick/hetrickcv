
#include "HCVChaos.h"
#include "math.hpp"

void HCVFBSineChaos::generate()
{
    float nextX = std::sin((indexX * lastY) + (feedback * lastX));
    float nextY = (phaseX * lastY + phaseInc);
    if(brokenMode)
    {
        nextY = reaktorDivMod(nextY, TWO_PI);
    }
    else
    {
        nextY = std::fmod(nextY, TWO_PI);
    }
    lastX = nextX;
    lastY = nextY;

    outX = lastX;
    outY = lastY * M_1_2PI;
}

////////////////////
///////////////////

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

////////////////////
///////////////////

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
    out1 = uniToBi(lastX);
    out2 = out1 * -1.0f;
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

//////////////////
//////////////////

void HCVLCCMap::generate()
{
    float base = (lastOut * chaosAmountA) + chaosAmountB;
    float unscaled = fmod(base, chaosAmountC);
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

///////////////////
///////////////////

void HCVDeJongMap::generate()
{
    float nextX = std::sin(lastX * chaosAmountC) - std::cos(lastY * chaosAmountD);
    float nextY = std::sin(lastY * chaosAmountA) - std::cos(lastX * chaosAmountB);

    lastX = nextX;
    lastY = nextY;

    outX = rack::math::clamp(lastX * 0.5f, -1.0f, 1.0f);
    outY = rack::math::clamp(lastY * 0.5f, -1.0f, 1.0f);
    outZ = outX * outY;
}

void HCVLatoocarfianMap::generate()
{
    float nextX = (std::sin(lastX * chaosAmountB) * chaosAmountC) + std::cos(lastY * chaosAmountB);
    float nextY = (std::sin(lastY * chaosAmountA) * chaosAmountD) + std::sin(lastX * chaosAmountA);

    lastX = nextX;
    lastY = nextY;

    outX = rack::math::clamp(lastX * 0.5f, -1.0f, 1.0f);
    outY = rack::math::clamp(lastY * 0.5f, -1.0f, 1.0f);
    outZ = outX * outY;
}

void HCVCliffordMap::generate()
{
    float nextX = (std::cos(lastX * chaosAmountA) * chaosAmountC) + std::sin(lastY * chaosAmountA);
    float nextY = (std::cos(lastY * chaosAmountB) * chaosAmountD) + std::sin(lastX * chaosAmountB);

    lastX = nextX;
    lastY = nextY;

    outX = rack::math::clamp(lastX * 0.5f, -1.0f, 1.0f);
    outY = rack::math::clamp(lastY * 0.5f, -1.0f, 1.0f);
    outZ = outX * outY;
}

void HCVPickoverMap::generate()
{
    float nextX = std::sin(lastY * chaosAmountA) - (std::cos(lastX * chaosAmountB) * lastZ);
    float nextY = (std::sin(lastX * chaosAmountC) * lastZ) - std::cos(lastY * chaosAmountD);
    float nextZ = std::sin(lastX) * 0.5f;

    lastX = nextX;
    lastY = nextY;
    lastZ = nextZ;

    outX = rack::math::clamp(lastX * 0.5f, -1.0f, 1.0f);
    outY = rack::math::clamp(lastY * 0.5f, -1.0f, 1.0f);
    outZ = rack::math::clamp(lastZ, -1.0f, 1.0f);
}

void HCVLorenzMap::generate()
{
    double nextX = ((lastY - lastX) * chaosAmountB) * chaosAmountA + lastX;
    double nextY = (((chaosAmountC - lastZ) * lastX) - lastY) * chaosAmountA + lastY;
    double nextZ = ((lastX * lastY) - (lastZ * chaosAmountD)) * chaosAmountA + lastZ;

    lastX = nextX;
    lastY = nextY;
    lastZ = nextZ;

    outX = rack::math::clamp(lastX * 0.02f, -1.0f, 1.0f);
    outY = rack::math::clamp(lastY * 0.02f, -1.0f, 1.0f);
    outZ = rack::math::clamp(lastZ * 0.02f, -1.0f, 1.0f);
}

void HCVRosslerMap::generate()
{
    float nextX = (-lastY - lastZ) * chaosAmountA + lastX;
    float nextY = ((lastY * chaosAmountB) + lastX) * chaosAmountA + lastY;
    float nextZ = ((lastX - chaosAmountD) * lastZ + chaosAmountC) * chaosAmountA + lastZ;

    lastX = rack::math::clamp(nextX, -20.0f, 20.0f);
    lastY = rack::math::clamp(nextY, -20.0f, 20.0f);
    lastZ = rack::math::clamp(nextZ, -20.0f, 20.0f);

    outX = lastX * 0.05f;
    outY = lastY * 0.05f;
    outZ = lastZ * 0.05f;
}

void HCVTinkerbellMap::generate()
{
    float nextX = ((lastX * lastX) - (lastY * lastY)) + ((chaosAmountA * lastX) + (chaosAmountB * lastY));
    float nextY = (2.0f * lastX * lastY) + ((chaosAmountC * lastX) + (chaosAmountD * lastY));

    if(immortal)
    {
        if(nextX == 0.0f) nextX = randomGen.whiteNoise();
        if(nextY == 0.0f) nextY = randomGen.whiteNoise();
    }

    lastX = rack::math::clamp(nextX, -1.0f, 1.0f);
    lastY = rack::math::clamp(nextY, -1.0f, 1.0f);

    outX = lastX;
    outY = lastY;
    outZ = outX * outY;
}

void HCVFitzhughNagumoMap::generate()
{
    float nextU = ((lastU - (lastU * lastU * lastU * 0.33333f)) - lastW) * chaosAmountA + lastU;
    if(std::abs(nextU) > 1.0f)
    {
        float mod = std::fmod(nextU - 1.0f, 4.0f);
        nextU = std::abs(mod - 2.0f) - 1.0f;
    }
    float nextW = (((lastU * chaosAmountD) + chaosAmountC) - lastW) * chaosAmountB;

    lastU = nextU;
    lastW = nextW;

    outX = rack::math::clamp(lastU, -1.0f, 1.0f);
    outY = rack::math::clamp(lastW, -1.0f, 1.0f);
    outZ = outX * outY;
}