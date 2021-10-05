#pragma once

class HCVSampleRate
{
public:
    HCVSampleRate()
    {

    }

    bool readyForNextSample()
    {
        sampleRateCounter += sampleRateFactor;
        
        if (sampleRateCounter >= 1.0f)
        {
            sampleRateCounter -= 1.0f;
            return true;
        }
        
        return false;
    }

    void setSampleRateFactor(float _factor)
    {
        sampleRateFactor = _factor;
    }

    float getSampleRateFactor()
    {
        return sampleRateFactor;
    }

    void reset()
    {
        sampleRateCounter = 1.0f;
    }

private:
    float sampleRateCounter = 0.0f;
    float sampleRateFactor = 1.0f;
};

class HCVSRateInterpolator
{
public:
    HCVSRateInterpolator()
	{

	}

	void setSRFactor(float _srFactor)
	{
		srFactor = std::max(_srFactor, 0.00000001f);
	}

	void setTargetValue(float _input)
	{
		targetValue = _input;
		diff = targetValue - currentValue;
		startValue = currentValue;
		factor = 0.0f;
	}

	float update()
	{
		factor = std::min(1.0f, factor + srFactor);
		currentValue = factor*diff + startValue;
		return currentValue;
	}

	float operator()()
	{
		return update();
	}

private:
    float diff = 0.0f, factor = 0.0f, currentValue = 0.0f, targetValue = 0.0f, startValue = 0.0f;
	float srFactor = 1.0;
};