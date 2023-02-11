#pragma once

#include <winrt/Windows.UI.h>

class HSVColorHelper
{
public:
	static const winrt::Windows::UI::Color HSVtoColor(float h, float s, float v);
};