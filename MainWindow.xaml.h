// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"
using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;
using namespace Microsoft::Graphics;

namespace winrt::ModernLife::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void CanvasControl_Draw(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl  const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
