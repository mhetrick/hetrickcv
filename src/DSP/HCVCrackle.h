#pragma once

#include "HCVRandom.h"
#include "math.hpp"

class CrackleGen
{
public:

    CrackleGen()
    {
        reset();
    }

    void reset()
    {
        y1 = rand.nextFloat();
        y2 = 0.0f;
        lasty1 = 0.0f;
    }

    float generate()
    {
        float y0;
        if(brokenMode)
        {
            y0 = std::abs(y1 * density - y2 - 0.05f);
            y2 = y1;
            y1 = lasty1;
            lasty1 = y0;
            lasty1 = rack::math::clamp(lasty1, -1.0f, 1.0f);
        }
        else
        {
            y0 = std::abs(y1 * density - y2 - 0.05f);
            y2 = y1;
            y1 = y0;
        }

        y0 = rack::math::clamp(y0, -1.0f, 1.0f);

        return y0;
    }

    float density = 1.0f;
    bool brokenMode = false;

private:
    float y1 = 0.0f, y2 = 0.0f, lasty1 = 0.0f;
    HCVRandom rand;
    
};

class HCVCrackle
{
public:
    HCVCrackle()
    {
        reset();
    }

    void reset()
    {
        crackleL.reset();
        crackleR.reset();
    }

    void setDensity(double _density)
    {
        float densityScaled = (_density * _density * _density) + 1.0;
        crackleL.density = densityScaled;
        crackleR.density = densityScaled;
    }

    float generate()
    {
        outL = crackleL.generate();
        return outL;
    }

    void generateStereo()
    {
        outL = crackleL.generate();
        outR = crackleR.generate();
    }

    void setBrokenMode(bool _brokenMode)
    {
        crackleL.brokenMode = _brokenMode;
        crackleR.brokenMode = _brokenMode;
    }

    float outL = 0.0f, outR = 0.0f;

private:
    CrackleGen crackleL, crackleR;
};