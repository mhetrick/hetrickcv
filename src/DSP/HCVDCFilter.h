#pragma once

#include "Gamma/Filter.h"

class HCVDCFilter
{
public:

    HCVDCFilter()
    {

    }

    float process(float _input)
    {
        return dcFilter(_input);
    }

    void setEnabled(bool _enabled)
    {
        
    }

private:
    gam::BlockDC<> dcFilter;
};