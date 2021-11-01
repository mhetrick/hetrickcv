#pragma once

constexpr auto HALF_PI = 1.57079632679f;
constexpr auto PI = 3.14159265359f;
constexpr auto TWO_PI = 6.28318530718f;

template <typename T = float>
inline T LERP(const float _amountOfA, const T _inA, const T _inB)
{
    return ((_amountOfA*_inA)+((1.0f-_amountOfA)*_inB));
}

template <typename T = float>
inline T uniToBi(const T _in)
{
    return (_in - T(0.5)) * 2.0;
}

template <typename T = float>
inline T SIMDLERP(const T _amountOfA, const T _inA, const T _inB)
{
    return ((_amountOfA*_inA)+((1.0f-_amountOfA)*_inB));
}