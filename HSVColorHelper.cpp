#include "pch.h"

#include "HSVColorHelper.h"

#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.UI.h>
#include <deps/gsl/include/gsl/gsl>
#include <directxmath.h>

#include "Log.h"

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

    //ML_TRACE("HSV color is: {}, {}, {}", h, s, v);
    const DirectX::XMFLOAT4 hsv{h / 360.0f, s, v, 0.0f };
    const DirectX::XMVECTOR hsvv {DirectX::XMLoadFloat4(&hsv)};
    const DirectX::XMVECTOR rgbv {DirectX::XMColorHSVToRGB(hsvv)};
    DirectX::XMFLOAT4A rgb;
    DirectX::XMStoreFloat4A(&rgb, rgbv);

    rgb.x = rgb.x * 255.0f;
    rgb.y = rgb.y * 255.0f;
    rgb.z = rgb.z * 255.0f;

    winrt::Windows::UI::Color colorDX = winrt::Microsoft::UI::ColorHelper::FromArgb(255, (uint8_t)std::lround(rgb.x), (uint8_t)std::lround(rgb.y), (uint8_t)std::lround(rgb.z));
    //ML_TRACE("DX color is: {}, {}, {}", rgb.x, rgb.y, rgb.z);

    return colorDX;


    /*h /= 60;
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

    winrt::Windows::UI::Color colorMe = winrt::Microsoft::UI::ColorHelper::FromArgb(255, r, g, b);

    ML_TRACE("My color is: {}, {}, {}", r, g, b);*/
}

