﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

#include "Board.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::Graphics;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;

namespace winrt::ModernLife::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        int32_t MyProperty();
        void MyProperty(int32_t value);
        //CanvasControl theCanvas;
        CanvasRenderTarget back{ nullptr };
        Board board{ nullptr };
        
        void CanvasControl_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void RenderOffscreen(CanvasControl const& sender);
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
