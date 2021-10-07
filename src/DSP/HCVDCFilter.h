#pragma once

#include "Gamma/Filter.h"
#include "HCVSlewedCrossfader.h"

template <typename T = float>
class HCVDCFilterT
{
public:

    HCVDCFilterT()
    {

    }

    T operator()(T _input)
    {
        return process(_input);
    }

    T process(T _input)
    {
        return crossfader(_input, dcFilter(_input));
    }

    void setEnabled(bool _enabled)
    {
        crossfader.setActive(_enabled);
    }

    void setFader(float _fader)
    {
        crossfader.setFader(_fader);
    }

private:
    gam::BlockDC<T> dcFilter;
    HCVSlewedCrossfaderT<T> crossfader;
};

typedef HCVDCFilterT<float> HCVDCFilter;