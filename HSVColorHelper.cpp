#include "pch.h"
#include "HSVColorHelper.h"

#include <winrt/Windows.UI.h>
#include "gsl/include/gsl"

// Adapted from https://www.cs.rit.edu/~ncs/color/t_convert.html#RGB%20to%20XYZ%20&%20XYZ%20to%20RGB
[[nodiscard]] winrt::Windows::UI::Color HSVColorHelper::HSVtoColor(float h, float s, float v)
{
    if (h > 360.0f)
    {
        return winrt::Windows::UI::Colors::Black();
    }
    if (s == 0)
    {
        return winrt::Windows::UI::Colors::Black();
    }

    h /= 60;
    const int i{ gsl::narrow_cast<int>(std::floor(h)) };
    const float f{ h - i };
    const float p{ v * (1 - s) };
    const float q{ v * (1 - s * f) };
    const float t{ v * (1 - s * (1 - f)) };

    float dr{ 0.0f };
    float dg{ 0.0f };
    float db{ 0.0f };

    switch (i)
    {
    case 0:
        dr = v;
        dg = t;
        db = p;
        break;
    case 1:
        dr = q;
        dg = v;
        db = p;
        break;
    case 2:
        dr = p;
        dg = v;
        db = t;
        break;
    case 3:
        dr = p;
        dg = q;
        db = v;
        break;
    case 4:
        dr = t;
        dg = p;
        db = v;
        break;
    case 5:
        dr = v;
        dg = p;
        db = q;
        break;
    default:
        dr = v;
        dg = v;
        db = v;
        break;
    }

    const uint8_t r{ gsl::narrow_cast<uint8_t>(dr * 255) };
    const uint8_t g{ gsl::narrow_cast<uint8_t>(dg * 255) };
    const uint8_t b{ gsl::narrow_cast<uint8_t>(db * 255) };

    return winrt::Windows::UI::ColorHelper::FromArgb(255, r, g, b);
}

