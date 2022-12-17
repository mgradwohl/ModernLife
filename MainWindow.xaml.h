// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

#include "Board.h"
#include "fpscounter.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::Graphics;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;

#ifdef _DEBUG
    constexpr int maxage = 1000;
    constexpr bool drawgrid = false;
    constexpr bool drawstats = true;
    constexpr bool singlerenderer = false;
#else
    constexpr int maxage = 1000;
    constexpr bool drawgrid = false;
    constexpr bool drawstats = true;
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
        int32_t SeedPercent() const;
        void SeedPercent(int32_t value);
        int16_t BoardWidth() const;
        void BoardWidth(int16_t value);

        void CanvasControl_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void RenderOffscreen(CanvasControl const& sender);
        void DrawInto(CanvasDrawingSession& ds, uint16_t sx, uint16_t ex, float width);
        Windows::UI::Color GetCellColor(uint16_t age) const;
        Windows::UI::Color GetCellColor2(uint16_t age);
        Windows::UI::Color GetCellColor3(uint16_t age);
        Windows::UI::Color GetCellColor4(uint16_t age);
        Windows::UI::Color GetCellColor5(uint16_t age);

        void theCanvasStatsContent_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
        void OnTick(IInspectable const& sender, IInspectable const& event);
        hstring GetRandPercentText(double_t value);
        hstring GetBoardWidthText(double_t value);
        void GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        Windows::UI::Color HSVtoRGB2(double H, double S, double V);

        winrt::event_token PropertyChanged(PropertyChangedEventHandler const& value)
        {
            return m_propertyChanged.add(value);
        }

        void PropertyChanged(winrt::event_token const& token)
        {
            m_propertyChanged.remove(token);
        }

        FPScounter fps;

    private:
        CanvasRenderTarget _back{ nullptr };
        std::mutex lockbackbuffer;
        std::mutex lockboard;
        Board board{ nullptr };

        DispatcherQueueController _controller{ nullptr };
        Microsoft::UI::Dispatching::DispatcherQueue _queue{ nullptr };
        DispatcherQueueTimer _timer{ nullptr };
        winrt::event_token _registrationtoken;
        bool _tokeninit = false;

        int32_t _randompercent = 30;
        int16_t _boardwidth = 200;

        winrt::event<PropertyChangedEventHandler> m_propertyChanged;
        bool _colorinit = false;
        std::vector<Windows::UI::Color> vecColors;
        int _speed = 30;
    public:
        void speedClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnWindowActivate(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const& args);
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
