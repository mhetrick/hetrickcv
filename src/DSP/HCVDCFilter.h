#pragma once

#include "Gamma/Filter.h"
#include "HCVSlewedCrossfader.h"

class HCVDCFilter
{
public:

    HCVDCFilter()
    {

    }

    float operator()(float _input)
    {
        return process(_input);
    }

    float process(float _input)
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
    gam::BlockDC<> dcFilter;
    HCVSlewedCrossfader crossfader;
};