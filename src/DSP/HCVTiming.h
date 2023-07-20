#pragma once

#include "rack.hpp"
#include "engine/Engine.hpp"
#include "dsp/digital.hpp"
#include "Gamma/Domain.h"
#include "HCVFunctions.h"

class HCVClockSync
{
public:

    void processGateClockInput(float _clockIn)
    {
        const float sampleTime = gam::Domain::master().ups();
        clockTimer.process(sampleTime);
        if(clockTrigger.process(_clockIn))
        {
            float newClockFreq = 1.f / clockTimer.getTime();
			clockTimer.reset();
            clockFreq = newClockFreq;
            //TODO: Maybe clamp clock freq
        }
    }

    float getBaseClockFreq() const
    {
        return clockFreq;
    }

private:
    float clockFreq = 1.0f;
    rack::dsp::Timer clockTimer;
    rack::dsp::SchmittTrigger clockTrigger;
};

//basically a wrapper around dsp::PulseGenerator
class HCVTriggeredGate
{
public:

    HCVTriggeredGate(float _timeInSeconds = 0.001f)
    {
        setTimeInSeconds(_timeInSeconds);
        reset();
    }

    bool process()
    {
		return gate.process(APP->engine->getSampleTime());
	}
    bool process(bool _trigger)
    {
        if(schmittBoolean.process(_trigger)) trigger();
        return gate.process(APP->engine->getSampleTime());
    }
    bool process(float _comparator)
    {
        if(schmitt.process(_comparator)) trigger();
        return gate.process(APP->engine->getSampleTime());
    }

    void trigger() 
    {
		gate.trigger(gateLengthInSeconds);
	}

    void reset()
    {
        gate.reset();
    }

    void setTimeInMilliseconds(float _msTime)
    {
        gateLengthInSeconds = _msTime * 0.001f; 
    }
    void setTimeInSeconds(float _seconds)
    {
        gateLengthInSeconds = _seconds;
    }

private:
    float gateLengthInSeconds = 0.001f;
    rack::dsp::PulseGenerator gate;
    rack::dsp::SchmittTrigger schmitt;
    rack::dsp::BooleanTrigger schmittBoolean;
};

class HCVTriggerDelay
{
public:
    HCVTriggerDelay(float _delayTimeInSeconds = 0.001f, float _gateTimeInSeconds = 0.001f)
    {
        delayedGate.setTimeInSeconds(_gateTimeInSeconds);
        delayTime = _delayTimeInSeconds;

        reset();
    }

    bool process()
    {
        bool delayTrig = timer.process(APP->engine->getSampleTime()) >= delayTime;
        bool triggered = schmitt.process(firstTrigger && delayTrig);
        zeroTrigger = false;
        return delayedGate.process(triggered);
    }

    void trigger()
    {
        firstTrigger = true;
        schmitt.state = false;
        timer.reset();
    }

    void reset()
    {
        schmitt.reset();
        delayedGate.reset();
        timer.reset();
        firstTrigger = false;
        zeroTrigger = false;
    }

    void setGateTimeInSeconds(float _gateTime)
    {
        delayedGate.setTimeInSeconds(_gateTime);
    }
    void setGateTimeInMilliseconds(float _gateTime)
    {
        delayedGate.setTimeInMilliseconds(_gateTime);
    }
    void setDelayTimeInSeconds(float _delayTime)
    {
        delayTime = _delayTime;
    }

private:
    float delayTime;
    HCVTriggeredGate delayedGate;
    rack::dsp::BooleanTrigger schmitt;
    rack::dsp::Timer timer;
    bool firstTrigger = false;
    bool zeroTrigger = false; //for delay == 0.0ms
};
