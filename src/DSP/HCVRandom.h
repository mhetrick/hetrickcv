#pragma once

#include "Gamma/rnd.h"
#include "HCVFunctions.h"
#include "math.hpp"

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
        return gam::rnd::gaussian(gamRand) * 0.3;
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

class HCVGrayNoise
{
public:
	float operator()()
	{
		auto newNoise = int(randGen.nextFloat() * 31.0) & 31;
		newNoise = 1 << newNoise;
		lastNoise = newNoise ^ lastNoise;

		const auto output = lastNoise * 4.65661287308e-10f;
		return rack::math::clamp(uniToBi(output), -1.0f, 1.0f);
	}

private:
	int lastNoise = 0;
	HCVRandom randGen;
};