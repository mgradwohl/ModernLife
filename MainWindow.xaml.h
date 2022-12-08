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

#ifdef _DEBUG
    constexpr int cellcount = 250;
    constexpr int maxage = 1000;
    constexpr bool drawgrid = false;
    constexpr bool drawstats = true;
    constexpr bool singlerenderer = false;
#else
    constexpr int cellcount = 250;
    constexpr int maxage = 1000;
    constexpr bool drawgrid = false;
    constexpr bool drawstats = false;
    constexpr bool singlerenderer = false;
#endif

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
        void RenderOffscreen(CanvasControl const& sender);
        void DrawInto(CanvasDrawingSession& ds, int sx, int ex, float width);
        Windows::UI::Color GetCellColor(const Cell& cell);
        void theCanvasDebug_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
        void OnTick(IInspectable const& sender, IInspectable const& event);
    private:
        CanvasRenderTarget _back{ nullptr };
        std::mutex lockbackbuffer;
        std::mutex lockboard;
        Board board{ nullptr };

        DispatcherQueueController _controller{ nullptr };
        Microsoft::UI::Dispatching::DispatcherQueue _queue{ nullptr };
        DispatcherQueueTimer _timer{ nullptr };
        float _randompercent = 0.3f;
    public:
        void GoButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestartButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void sliderPop_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
