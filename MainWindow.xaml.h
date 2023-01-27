// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"
#include <winrt/Windows.Graphics.Display.h>

#include "Board.h"
#include "fpscounter.h"
#include "TimerHelper.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::Graphics;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;

constexpr int maxage = 10000;

namespace winrt::ModernLife::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow() noexcept;
        void InitializeComponent();
        void StartGameLoop();

        int32_t SeedPercent() const noexcept;
        void SeedPercent(int32_t value);
        int16_t BoardWidth() const noexcept;
        void BoardWidth(int16_t value);

        void CanvasControl_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void RenderOffscreen(CanvasControl const& sender);
        void DrawHorizontalRows(const CanvasDrawingSession& ds, uint16_t sx, uint16_t ex);

        void speedClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnWindowActivate(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const& args);
        void theCanvas_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);

        Windows::UI::Color GetCellColorHSV(uint16_t age);
        Windows::UI::Color GetOutlineColorHSV(uint16_t age);
        Windows::UI::Color HSVtoColor(float h, float s, float v);

        void BuildSpriteSheet(const CanvasDevice& device);


        void theCanvasStatsContent_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, winrt::Windows::Foundation::IInspectable const&);
        hstring GetRandPercentText(double_t value);
        hstring GetBoardWidthText(double_t value);
        void GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SetupRenderTargets();
        //void OnDpiChanged(Windows::Graphics::Display::DisplayInformation const& , IInspectable const&);
        void theCanvas_CreateResources(CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args);
        void SetBestCanvasSizes() noexcept;
        void Window_Closed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept;
        void ruleClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        winrt::event_token PropertyChanged(PropertyChangedEventHandler const& value)
        {
            return m_propertyChanged.add(value);
        }

        void PropertyChanged(winrt::event_token const& token)
        {
            m_propertyChanged.remove(token);
        }

        FPScounter fps{ nullptr };
		TimerHelper timer{ 30, true };

    private:
        CanvasRenderTarget _backbuffer{ nullptr };
        CanvasRenderTarget _spritesheet{ nullptr };
        std::mutex lockbackbuffer;
        std::mutex lockboard;
        Board _board{ nullptr };

        float _widthCellDest{};
        float _canvasSize{};
        float _dpi{ 96.0f };

        int32_t _randompercent{30};
        int32_t _ruleset{ 1 };
        int16_t _boardwidth{ 200 };
        int32_t _bestcanvassize{ 1000 };
        float _bestbackbuffersize = 3000.0f;

        winrt::event<PropertyChangedEventHandler> m_propertyChanged;

    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
