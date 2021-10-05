#pragma once

#include "Gamma/rnd.h"

class HCVRandom
{
public:
    HCVRandom(int _seed = 0)
    {
        if(_seed) seed(_seed);
        else seed(gam::rnd::getSeed());
    }

    void seed(const int seed)
    {
        gamRand.val = seed;
    }

    // [0, 1]
    float nextFloat() const
    {
        return gam::rnd::uni_float(gamRand);
    }

    //[-1, 1]
    float whiteNoise() const
    {
        return (nextFloat() - 0.5f) * 2.0f;
    }
    
    float nextGaussian() const
    {
        return gam::rnd::gaussian(gamRand);
    }

    bool nextProbability(float _prob) const
	{
		return gam::rnd::prob(gamRand, _prob);
	}

    bool nextBoolean() const
    {
        return nextProbability(0.5);
    }

	float operator()() const
	{
        return nextFloat();
	}

private:
    gam::RNGMulCon gamRand;
};