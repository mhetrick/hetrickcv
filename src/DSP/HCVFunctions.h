#pragma once

constexpr auto HALF_PI = 1.57079632679f;
constexpr auto PI = 3.14159265359f;
constexpr auto TWO_PI = 6.28318530718f;

inline float LERP(const float _amountOfA, const float _inA, const float _inB)
{
    return ((_amountOfA*_inA)+((1.0f-_amountOfA)*_inB));
}