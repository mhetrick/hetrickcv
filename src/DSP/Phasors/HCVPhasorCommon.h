
#pragma once

#include "Gamma/scl.h"

static constexpr float HCV_PHZ_UPSCALE = 10.0f;
static constexpr float HCV_PHZ_DOWNSCALE = 0.1f;
static constexpr float HCV_PHZ_GATESCALE = 10.0f;

static float scaleAndWrapPhasor(float _input)
{
    return gam::scl::wrap(_input * HCV_PHZ_DOWNSCALE);
}