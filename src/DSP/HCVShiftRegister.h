#pragma once

#include "dsp/digital.hpp"
#include <vector>
#include "HCVRandom.h"
#include "HCVFunctions.h"

template <typename T = float>
class HCVShiftRegister
{
public:
    HCVShiftRegister(int size = 8)
    {
        dataRegister.resize(size);

        for(auto dataReg : dataRegister)
        {
            dataReg = T(0);
        }
    }

    virtual void advanceRegister(T _input)
    {
        for (int i = dataRegister.size() - 1; i > 0; i--)
        {
            dataRegister[i] = dataRegister[i-1];
        }

        dataRegister[0] = _input;
    }

    virtual void advanceRegisterFrozen()
    {
        T temp = dataRegister[dataRegister.size() - 1];

        for (int i = dataRegister.size() - 1; i > 0; i--)
        {
            dataRegister[i] = dataRegister[i-1];
        }
        dataRegister[0] = temp;
    }

    std::vector<T> dataRegister;

private:
    
};

class HCVRungler : public HCVShiftRegister<bool>
{
public:

    virtual void advanceRegister(bool _input) override
    {
        HCVShiftRegister::advanceRegister(_input);
        calculateRunglerOut();
    }

    void advanceRegisterFrozen() override
    {
        HCVShiftRegister::advanceRegisterFrozen();
        calculateRunglerOut();
    }

    void calculateRunglerOut()
    {
        float nextRungler = (dataRegister[5] ? 32.0f : 0.0f);
		nextRungler = nextRungler + (dataRegister[6] ? 64.0f : 0.0f);
		nextRungler = nextRungler + (dataRegister[7] ? 128.0f : 0.0f);
		nextRungler = (nextRungler/255.0f);

        runglerOut = nextRungler;
    }

    float getRunglerOut()
    {
        return runglerOut;
    }

private:
    float runglerOut = 0.0f;
};

class HCVLFSRNoise : public HCVShiftRegister<float>
{
public:
    float operator()()
    {
        advanceRegister(randGen.nextBoolean());

        float noiseValue = dataRegister[0] ? 1.0f : 0.0f;
        noiseValue += dataRegister[1] ? 2.0f : 0.0f;
        noiseValue += dataRegister[2] ? 4.0f : 0.0f;
        noiseValue += dataRegister[3] ? 8.0f : 0.0f;
        noiseValue += dataRegister[4] ? 16.0f : 0.0f;
        noiseValue += dataRegister[5] ? 32.0f : 0.0f;
        noiseValue += dataRegister[6] ? 64.0f : 0.0f;
        noiseValue += dataRegister[7] ? 128.0f : 0.0f;

        noiseValue = noiseValue/255.0f;
        noiseValue = uniToBi(noiseValue);

        return noiseValue;
    }

private:
    HCVRandom randGen;
};