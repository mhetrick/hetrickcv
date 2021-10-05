#pragma once

#include <cstdlib>
#include "HCVRandom.h"

class HCVGingerbreadMap
{
public:
    HCVGingerbreadMap()
    {
        reset();
    }

    float generate()
    {
        auto nextX = 1.0f - lastY + std::abs(lastX);
        auto nextY = lastX;

        lastX = nextX;
        lastY = nextY;

        return lastX;
    }

    void reset()
    {
        lastX = randomGen.whiteNoise() * 4.0;
        lastY = randomGen.whiteNoise() * 4.0;
    }

private:
    float lastX = 1.2, lastY = 0.124098;
    HCVRandom randomGen;
};
