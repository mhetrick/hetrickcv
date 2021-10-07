#pragma once

#include "Gamma/Domain.h"
#include <dsp/common.hpp>

//rewrite of VCV's slew limiter as a Gamma sample rate observer
//this is more optimized for slew limiters with fixed slew times
template <typename T = float>
class HCVSlewLimiter : public gam::DomainObserver
{
public:
    
	HCVSlewLimiter(T _rise = T(100.0f), T _fall = T(100.0f))
	{
		setRiseFall(_rise, _fall);
	}

	void reset() 
    {
		out = 0.f;
	}

	void setRiseFall(T _rise, T _fall) 
    {
		rise = _rise;
		fall = _fall;
		calculateScaledTimes();
	}

	void calculateScaledTimes()
	{
		riseScaled = rise * domain()->ups();
		fallScaled = fall * domain()->ups();
	}

	//when the sample rate changes, calculate new scaled times;
	void onDomainChange(double /*ratioSPU*/) override
	{
		calculateScaledTimes();
	}

	T process(T in) 
    {
		out = simd::clamp(in, out - fallScaled, out + riseScaled);
		return out;
	}

private:
    T out = 0.f;
	T rise = 0.f;
	T fall = 0.f;
	T riseScaled = 0.f;
	T fallScaled = 0.f;
};