#pragma once

#include "Gamma/Oscillator.h"
#include "dsp/digital.hpp"
#include "HCVRandom.h"

//Phasor processing tricks learned from "Generating Sound and Organizing Time"
//by Graham Wakefield and Gregory Taylor. Highly recommended!
//Also inspired by Bitwig Grid and Toybox Nano Pack.

static constexpr float HCV_PHZ_UPSCALE = 10.0f;
static constexpr float HCV_PHZ_DOWNSCALE = 0.1f;

class HCVPhasorBase
{
public:

    virtual void reset() = 0;
    virtual void setFreqDirect(float _freq) = 0;
    virtual float getCurrentPhase() = 0;
    virtual bool phasorFinishedThisSample() = 0;

    void setFrozen(bool _isFrozen) { frozenMult = _isFrozen ? 0.0f : 1.0f; }
    void setOutputScalar(float _scalar){ outputScalar = _scalar; } 
    void setPulseWidth(float _pulseWidth){ pulseWidth = _pulseWidth; }
    void setJitterDepth(float _jitterDepth){ jitterDepth = _jitterDepth; }
    void setPulsesPerCycle(int _pulsesPerCycle){ pulsesPerCycle = _pulsesPerCycle; }
    float getJitterSample() { return jitterValue; }

    bool processGateResetInput(float _gateIn)
    {
        if(resetTrigger.process(_gateIn))
        {
            reset();
            return true;
        }

        return false;
    }

    float getPulse()
    {
        const float scaledPhase = getCurrentPhase() * pulsesPerCycle;
        const float stepWidth = scaledPhase - floorf(scaledPhase);
        return stepWidth < pulseWidth ? gateScale : 0.0f;
    }

    void updateRandomDepth()
    {
        if(phasorFinishedThisSample())
        {
            jitterValue = randomGen.whiteNoise();
        }
    }

protected:
    float clockFreq = 1.0f;
    float outputScalar = HCV_PHZ_UPSCALE;
    float gateScale = 5.0f;
    float pulseWidth = 0.5f;
    float frozenMult = 1.0f;
    float jitterDepth = 0.0f;
    float jitterValue = 0.0f;
    int pulsesPerCycle = 1;

    HCVRandom randomGen;
    rack::dsp::SchmittTrigger resetTrigger;
};

class HCVPhasor : public HCVPhasorBase
{
public:
    HCVPhasor()
    {
        setFreqDirect(2.0f);
    }

    float operator()()
    {
        phasor.freqMul(reverseMult * frozenMult + (jitterValue*jitterDepth));        
        updateRandomDepth();
        return phasor() * outputScalar;
    }

    void reset() override
    {
        phasor.reset();
        phasor.phase(phaseOffset);
    }

    void setPhaseOffset(float _newOffset)
    {
        float phaseDiff = _newOffset - phaseOffset;
        phasor.phaseAdd(phaseDiff);
        phaseOffset = _newOffset;
    }

    float getCurrentPhase() override { return phasor.phase(); }
    void setFreqDirect(float _freq) override { phasor.freq(_freq); }

    void setReversed(bool _isReversed) { reverseMult = _isReversed ? -1.0f : 1.0f; }
    bool phasorFinishedThisSample() override { return phasor.cycled();}

protected:
    gam::Sweep<> phasor;
    float phaseOffset = 0.0f;
    float reverseMult = 1.0f;
};

class HCVBurstPhasor : public HCVPhasorBase
{
public:

    HCVBurstPhasor()
    {
        setRepeats(1);
        setFreqDirect(2.0f);
    }

    float operator()()
    {
        phasor.freqMul(frozenMult + (jitterValue*jitterDepth));
        updateRandomDepth();
        return phasor() * outputScalar;
    }

    void reset() override
    {
        phasor.reset();
    }

    void setRepeats(int _repeats)
    {
        if(_repeats < 1) _repeats = 1;
        phasor.phsInc().repeats(_repeats);
    }

    float getCurrentPhase() override { return phasor.phase(); }
    void setFreqDirect(float _freq) override { phasor.freq(_freq); }
    bool phasorFinishedThisSample() override { return phasor.cycled(); }
    bool done() { return phasor.done(); }

protected:
    gam::Sweep<gam::phsInc::NShot> phasor;
};
