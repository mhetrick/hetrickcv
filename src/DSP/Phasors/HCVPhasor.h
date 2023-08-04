#pragma once

#include "Gamma/Oscillator.h"
#include "dsp/digital.hpp"
#include "../HCVRandom.h"
#include "HCVPhasorCommon.h"

//Phasor processing tricks learned from "Generating Sound and Organizing Time"
//by Graham Wakefield and Gregory Taylor. Highly recommended!
//Also inspired by Bitwig Grid and Toybox Nano Pack.

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
    float gateScale = HCV_PHZ_GATESCALE;
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

struct HCVBurst //modified Gamma NShot to add stopBurst
{
    HCVBurst(int _maxRepeats = 64)
    { 
        repeats(1);
        maxRepeats = _maxRepeats;
        mCount = _maxRepeats; //set count to max to not trigger on load.
    }
    
    void reset(){ mCount=0; }
    void stopBurst()
    {
        mCount = maxRepeats;
    }
    void setMaxRepeats(int _max)
    {
        maxRepeats = _max;
    }

    uint32_t operator()(uint32_t& pos, uint32_t inc){
        uint32_t prev = pos;
        pos += inc;
        
        // Check MSB goes from 1 to 0
        // TODO: works only for positive increments and non-zero mNumber
        if((~pos & prev) & 0x80000000){
            if(++mCount >= mRepeats) pos = 0xffffffff;
        }
        return pos;
    }
    
    bool done(uint32_t pos) const { return (mCount >= mRepeats) && (pos == 0xffffffff); }
    
    template <class T>
    T operator()(T v, T inc, T max, T min){
        v += inc;
        if(v >= max || v < min) ++mCount;
        return mCount < mRepeats ? gam::scl::wrap(v, max, min) : gam::phsInc::incClip(v, inc, max, min);
    }
    
    /// Set number of repetitions
    HCVBurst& repeats(uint32_t v){ mRepeats=v; return *this; }
    HCVBurst& number(uint32_t v){ mRepeats=v; return *this; }

protected:
    uint32_t mRepeats;
    uint32_t mCount;
    uint32_t maxRepeats;
};

class HCVBurstPhasor : public HCVPhasorBase
{
public:

    HCVBurstPhasor(int maxRepeats = 64)
    {
        phasor.phsInc().setMaxRepeats(maxRepeats);
        stopPhasor(); //set phasor to max repeats, then force current count to max.
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

    void stopPhasor()
    {
        phasor.phsInc().stopBurst();
        phasor.finish();
    }

    float getCurrentPhase() override { return phasor.phase(); }
    void setFreqDirect(float _freq) override { phasor.freq(_freq); }
    bool phasorFinishedThisSample() override { return phasor.cycled(); }
    bool done() { return phasor.done(); }

protected:
    gam::Sweep<HCVBurst> phasor;
};
