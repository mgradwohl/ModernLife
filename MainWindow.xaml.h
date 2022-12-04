// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

#include "Board.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::Graphics;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;

constexpr int cellcount = 250;

namespace winrt::ModernLife::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow();
        void StartGameLoop();

        int32_t MyProperty();
        void MyProperty(int32_t value);
        
        void CanvasControl_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        CanvasRenderTarget& GetBackBuffer();
        void RenderOffscreen(CanvasControl const& sender);
        void DrawInto(CanvasDrawingSession& ds, int sx, int ex, float width);
        Windows::UI::Color GetCellColor(const Cell& cell);
        void theCanvasDebug_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
        void OnTick(IInspectable const& sender, IInspectable const& event);
    private:
        CanvasRenderTarget _back{ nullptr };
        std::mutex lockbackbuffer;
        Board board{ nullptr };

        DispatcherQueueController _controller{ nullptr };
        Microsoft::UI::Dispatching::DispatcherQueue _queue{ nullptr };
        DispatcherQueueTimer _timer{ nullptr };
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
