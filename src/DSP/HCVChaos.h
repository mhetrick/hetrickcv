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

//////////////////
//////////////////

class HCVFBSineChaos
{
public:
    HCVFBSineChaos()
    {

    }

    void generate();

    float outX = 0.0f, outY = 0.0f;
    bool brokenMode = false;

    void setIndexX(double _indexX)
    {
        indexX = 5.0 * _indexX;
    }

    void setPhaseInc(double _phaseInc)
    {
        phaseInc = uniToBi(_phaseInc);
    }

    void setPhaseX(double _phaseX)
    {
        phaseX = (_phaseX * _phaseX * _phaseX * 2.0f) + 1.0f;
    }

    void setFeedback(double _feedback)
    {
        feedback = uniToBi(_feedback);
    }

private:
    float lastX = 0.0f, lastY = 0.0f;
    float indexX = 0.0f, phaseInc = 0.0f, phaseX = 0.0f, feedback = 0.0f;

    float reaktorDivMod(float _in, float _mod)
    {
        float baseDiv = std::floor(_in/_mod) * _mod;

        if(_in > 0.0f) return (_in - baseDiv);
        return (_in - baseDiv) * -1.0f;
    }
};

//////////////////
//////////////////

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

//////////////////////////
//////1 op Chaos
//////////////////////////

class HCVChaos1Op : public HCVChaosBase
{
public:
    virtual void setChaosAmount(const float _chaosAmount)
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

	void setChaosAmount(const float _chaosAmount) override
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

    void setChaosAmount(const float _chaosAmount) override final
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

    void setChaosAmount(const float _chaosAmount) override final
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

    void setChaosAmount(const float _chaosAmount) override final
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

//////////////////////////
//////2 op Chaos
//////////////////////////

class HCVChaos2Op : public HCVChaosBase
{
public:
    virtual void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB)
    {
        chaosAmountA = _chaosAmountA;
        chaosAmountB = _chaosAmountB;
    }
    float out1 = 0.0f, out2 = 0.0f;

protected:
    float chaosAmountA = 0.0f, chaosAmountB = 0.0f;
};

class HCVHenonMap : public HCVChaos2Op
{
public:
    HCVHenonMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB) override final
    {
        chaosAmountA = _chaosAmountA * 2.0f;
        chaosAmountB = _chaosAmountB;
    }

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise();
        lastY = randomGen.whiteNoise();
    }

private:
    float lastX = 0.0f, lastY = 0.0f;
};

class HCVHetrickMap : public HCVChaos2Op
{
public:
    HCVHetrickMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB) override final
    {
        chaosAmountA = _chaosAmountA + 1.0f;
        chaosAmountB = _chaosAmountB * 0.5f;
    }

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise();
        lastX2 = randomGen.whiteNoise();
    }

private:
    float lastX = 0.0f, lastX2 = 0.0f;
};

class HCVCuspMap : public HCVChaos2Op
{
public:
    HCVCuspMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB) override final
    {
        chaosAmountA = _chaosAmountA + 1.0f;
        chaosAmountB = _chaosAmountB + 1.0f;
    }

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise();
    }

private:
    float lastX = 0.0f;
};

class HCVGaussMap : public HCVChaos2Op
{
public:
    HCVGaussMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB) override final
    {
        chaosAmountA = _chaosAmountA * 0.4f;
        chaosAmountB = gam::scl::mapLin(_chaosAmountB, 0.0f, 1.0f, 0.001f, 0.5f);
    }

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise();
    }

private:
    float lastX = 0.0f;
};

class HCVMouseMap : public HCVChaos2Op
{
public:
    HCVMouseMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB) override final
    {
        chaosAmountA = 2.0 + (_chaosAmountA * 6.0);
        chaosAmountB = _chaosAmountB * -0.7;
    }

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise();
    }

private:
    float lastX = 0.0f;
};

//////////////////////////
//////3 op Chaos
//////////////////////////

class HCVChaos3Op : public HCVChaosBase
{
public:
    virtual void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB, const float _chaosAmountC)
    {
        chaosAmountA = _chaosAmountA;
        chaosAmountB = _chaosAmountB;
        chaosAmountC = _chaosAmountC;
    }
    float out = 0.0f;

protected:
    float chaosAmountA = 0.0f, chaosAmountB = 0.0f, chaosAmountC = 0.0f;
};

class HCVLCCMap : public HCVChaos3Op
{
public:
    HCVLCCMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB, const float _chaosAmountC) override final
    {
        chaosAmountA = (_chaosAmountA * _chaosAmountA) + 0.9f;
        chaosAmountB = _chaosAmountB * 0.3f;
        chaosAmountC = 0.0001f + (_chaosAmountC * 0.9999f);
    }

    void generate() override final;

    void reset() override final
    {
        lastOut = randomGen.nextFloat();
    }

private:
    float lastOut = 0.5f;
};

class HCVQuadraticMap : public HCVChaos3Op
{
public:
    HCVQuadraticMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosAmountA, const float _chaosAmountB, const float _chaosAmountC) override final
    {
        chaosAmountA = -3.4f - (_chaosAmountA * 0.6f);
        chaosAmountB = 3.4f + (_chaosAmountB * 0.6f);
        chaosAmountC = (_chaosAmountC * 0.6f) - 0.1f;
    }

    void generate() override final;

    void reset() override final
    {
        lastOut = randomGen.nextFloat();
    }

private:
    float lastOut = 0.5f;
};

//////////////////////////
//////4 op Chaos
//////////////////////////

class HCVChaos4Op : public HCVChaosBase
{
public:
    virtual void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD)
    {
        chaosAmountA = _chaosA;
        chaosAmountB = _chaosA;
        chaosAmountC = _chaosA;
        chaosAmountD = _chaosD;
    }
    float outX = 0.0f, outY = 0.0f, outZ = 0.0f;

protected:
    float chaosAmountA = 0.0f, chaosAmountB = 0.0f, chaosAmountC = 0.0f, chaosAmountD = 0.0f;
};

class HCVDeJongMap : public HCVChaos4Op
{
public:

    HCVDeJongMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = (_chaosA * TWO_PI) - PI;
        chaosAmountB = (_chaosB * TWO_PI) - PI;
        chaosAmountC = (_chaosC * TWO_PI) - PI;
        chaosAmountD = (_chaosD * TWO_PI) - PI;
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.nextFloat();
        lastY = randomGen.nextFloat();
    }


private:
    double lastX = 0.0f, lastY = 0.0f;
};

class HCVLatoocarfianMap : public HCVChaos4Op
{
public:

    HCVLatoocarfianMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = (_chaosA * TWO_PI) - PI;
        chaosAmountB = (_chaosB * TWO_PI) - PI;
        chaosAmountC = _chaosC + 0.5f;
        chaosAmountD = _chaosD + 0.5f;
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.nextFloat();
        lastY = randomGen.nextFloat();
    }


private:
    double lastX = 0.0f, lastY = 0.0f;
};

class HCVCliffordMap : public HCVChaos4Op
{
public:

    HCVCliffordMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = (_chaosA * TWO_PI) - PI;
        chaosAmountB = (_chaosB * TWO_PI) - PI;
        chaosAmountC = _chaosC + 0.5f;
        chaosAmountD = _chaosD + 0.5f;
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.nextFloat();
        lastY = randomGen.nextFloat();
    }


private:
    double lastX = 0.0f, lastY = 0.0f;
};

class HCVPickoverMap : public HCVChaos4Op
{
public:

    HCVPickoverMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = _chaosA * 5.0f;
        chaosAmountB = _chaosB * 5.0f;
        chaosAmountC = _chaosC * 5.0f;
        chaosAmountD = _chaosD * 5.0f;
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.nextFloat();
        lastY = randomGen.nextFloat();
        lastZ = randomGen.nextFloat();
    }


private:
    double lastX = 0.0f, lastY = 0.0f, lastZ = 0.0f;
};

class HCVLorenzMap : public HCVChaos4Op
{
public:

    HCVLorenzMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = gam::scl::mapLin(_chaosA, 0.0f, 1.0f, 0.001f, 0.01f);
        chaosAmountB = 4.0f + (_chaosB * 51.0f);
        chaosAmountC = 10.0f + (_chaosC * 40.0f);
        chaosAmountD = 0.4f + (_chaosD * 4.6f);
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.nextFloat();
        lastY = randomGen.nextFloat();
        lastZ = randomGen.nextFloat();
    }


private:
    double lastX = 0.0f, lastY = 0.0f, lastZ = 0.0f;
};

class HCVRosslerMap : public HCVChaos4Op
{
public:

    HCVRosslerMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = gam::scl::mapLin(_chaosA, 0.0f, 1.0f, 0.001f, 0.015f);
        chaosAmountB = _chaosB * 0.35f;
        chaosAmountC = 0.5f + (_chaosC * 0.5f);
        chaosAmountD = 1.0f + (_chaosD * 9.0f);
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.nextFloat();
        lastY = randomGen.nextFloat();
        lastZ = randomGen.nextFloat();
    }


private:
    double lastX = 0.0f, lastY = 0.0f, lastZ = 0.0f;
};

class HCVTinkerbellMap : public HCVChaos4Op
{
public:

    HCVTinkerbellMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = _chaosA;
        chaosAmountB = uniToBi(_chaosB);
        chaosAmountC = _chaosC * 4.0f;
        chaosAmountD = _chaosD;
    } 

    void generate() override final;

    void reset() override final
    {
        lastX = randomGen.whiteNoise();
        lastY = randomGen.whiteNoise();
    }

    bool immortal = true;

private:
    double lastX = 0.0f, lastY = 0.0f;
};

class HCVFitzhughNagumoMap : public HCVChaos4Op
{
public:

    HCVFitzhughNagumoMap()
    {
        reset();
    }

    void setChaosAmount(const float _chaosA, const float _chaosB, const float _chaosC, const float _chaosD) override final
    {
        chaosAmountA = _chaosA;
        chaosAmountB = _chaosA;
        chaosAmountC = _chaosA;
        chaosAmountD = _chaosD;
    } 

    void generate() override final;

    void reset() override final
    {
        lastU = randomGen.nextFloat();
        lastW = randomGen.nextFloat();
    }


private:
    double lastU = 0.0f, lastW = 0.0f;
};