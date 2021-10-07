#pragma once

#include <cstdlib>
#include "HCVRandom.h"
#include "HCVFunctions.h"

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

class HCVChaosBase
{
public:
	HCVChaosBase() = default;
    virtual ~HCVChaosBase() = default;

    virtual void reset() = 0;
    virtual void generate() = 0;

protected:

    HCVRandom randomGen;
};

class HCVChaos1Op : public HCVChaosBase
{
public:
    virtual void setChaos(const float _chaosAmount)
    {
        chaosAmount = _chaosAmount;
    }
    float out1 = 0.0f, out2 = 0.0f;

protected:
    float chaosAmount = 0.0f;
};

class HCVLogisticMap final: public HCVChaos1Op
{
public:
	HCVLogisticMap()
	{
		reset();
	}

	void setChaos(const float _chaosAmount) override
	{
		chaosAmount = 3.0f + _chaosAmount;
	}
    
    void reset() override final
    {
        lastValue = 0.6f;
    }

	void generate() override final;

private:
    float lastValue = 0.6f;
	const float lowLimit = 0.00001;
	const float upperLimit = 1.0f - lowLimit;
};

class HCVIkedaMap : public HCVChaos1Op
{
public:
    HCVIkedaMap()
    {
        HCVIkedaMap::reset();
    }

    void setChaos(const float _chaosAmount) override final
    {
        chaosAmount = gam::scl::mapLin(_chaosAmount, 0.0f, 1.0f, 0.79f, 0.87f);
    }

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise() * 5.0f;
        lastY = randomGen.whiteNoise() * 5.0f;
    }

private:
    float lastX = 0.0f, lastY = 0.0f;
};

class HCVTentMap : public HCVChaos1Op
{
public:
    HCVTentMap()
    {
        reset();
    }

    void setChaos(const float _chaosAmount) override final
    {
        chaosAmount = gam::scl::mapLin(_chaosAmount, 0.0f, 1.0f, 1.0001f, 1.999f);
    }

    void generate() override final;

    void reset() override final
    {
        out = randomGen.nextFloat();
    }

private:
    float out = 0.0;
};

class HCVStandardMap : public HCVChaos1Op
{
public:
    HCVStandardMap()
    {
        reset();
    }

    void setChaos(const float _chaosAmount) override final
    {
        chaosAmount = 8.0f * _chaosAmount;
    }

    void generate() override final;

    void reset() override final
    {
        lastP = randomGen.nextFloat();
        lastO = randomGen.nextFloat();
    }

private:
    static float scaleOutput(float _in)
    {
        static const float piRecip = 1.0f/PI;
        float out = _in;
        out = out - PI;
        out = out * piRecip * 0.4f;
        return out;
    }

    float lastP = 0.0, lastO = 0.0;
};
