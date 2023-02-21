#pragma once

#include <winrt/Windows.UI.h>

class HSVColorHelper
{
public:
	[[nodiscard]] static winrt::Windows::UI::Color HSVtoColor(float h, float s, float v);
};