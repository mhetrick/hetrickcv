#pragma once

#include "Gamma/Domain.h"
#include <dsp/common.hpp>

//rewrite of VCV's slew limiter as a Gamma sample rate observer

template <typename T = float>
class HCVSlewLimiter : public gam::DomainObserver
{
public:
    

	void reset() 
    {
		out = 0.f;
	}

	void setRiseFall(T rise, T fall) 
    {
		this->rise = rise;
		this->fall = fall;
	}

	T process(T deltaTime, T in) 
    {
		out = simd::clamp(in, out - fall * deltaTime, out + rise * deltaTime);
		return out;
	}

	T process(T in) 
    {
		return process(domain()->ups(), in);
	}

private:
    T out = 0.f;
	T rise = 0.f;
	T fall = 0.f;
};