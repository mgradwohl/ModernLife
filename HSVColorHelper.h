#pragma once

#include <winrt/Windows.UI.h>

class HSVColorHelper
{
public:

	// construct
	HSVColorHelper() = delete;
	HSVColorHelper(HSVColorHelper&) = delete;
	HSVColorHelper(HSVColorHelper&&) = delete;

	// copy/move
	HSVColorHelper& operator=(HSVColorHelper&) = delete;
	HSVColorHelper& operator=(HSVColorHelper&&) = delete;

	// destruct
	~HSVColorHelper() = delete;

	[[nodiscard]] static winrt::Windows::UI::Color HSVtoColor(float h, float s, float v);
};