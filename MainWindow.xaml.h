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

namespace winrt::ModernLife::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow() noexcept;
        void OnWindowActivate(IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const& args);
        void InitializeComponent();
        void SetMyTitleBar();
        void Window_Closed(IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept;
        void StartGameLoop();

        void OnPropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& args);
        int32_t SeedPercent() const noexcept;
        void SeedPercent(int32_t value);
        int32_t MaxAge() const noexcept;
        void MaxAge(int32_t value);
        bool ShowLegend() const noexcept;
        void ShowLegend(bool value);
        void BoardWidth(int16_t value);
        int16_t BoardWidth() const noexcept;
        hstring GetRandPercentText(double_t value);
        hstring GetBoardWidthText(double_t value);
        void speedClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void theCanvas_SizeChanged(IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
        void OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, IInspectable const&);
        void GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ruleClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void BuildSpriteSheet(const CanvasDevice& device);
        void CanvasControl_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void RenderOffscreen(CanvasControl const& sender);
        void DrawHorizontalRows(const CanvasDrawingSession& ds, uint16_t sx, uint16_t ex);
        void theCanvasStatsContent_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void SetupRenderTargets();
        void theCanvas_CreateResources(CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args);
        void SetBestCanvasandWindowSizes();

        Windows::UI::Color GetCellColorHSV(uint16_t age);
        Windows::UI::Color GetOutlineColorHSV(uint16_t age);
        Windows::UI::Color HSVtoColor(float h, float s, float v);

        winrt::event_token PropertyChanged(PropertyChangedEventHandler const& value)
        {
            return _propertyChanged.add(value);
        }

        void PropertyChanged(winrt::event_token const& token)
        {
            _propertyChanged.remove(token);
        }

        FPScounter fps{ nullptr };
		TimerHelper timer{ 30, true };

    private:
        CanvasRenderTarget _backbuffer{ nullptr };
        CanvasRenderTarget _spritesheet{ nullptr };
        CanvasRenderTarget _spriteOld{ nullptr };

        std::mutex lockbackbuffer;
        std::mutex lockboard;
        Board _board{ nullptr };

        float _widthCellDest{};
        float _canvasSize{};
        float _dpi{ 0.0f };

        bool _drawLegend{ false };
        int32_t _randompercent{30};
        int32_t _maxage{ 1000 };
        int32_t _ruleset{ 1 };
        int16_t _boardwidth{ 200 };
        float _bestcanvassize{ 1000 };
        // 6 units per cell, 500 cells per line
        float _idealbackbuffersize{ 3000.0f };
        float _bestbackbuffersize{ 3000.0f };

        winrt::event_token _propertyToken;
        winrt::event<PropertyChangedEventHandler> _propertyChanged;
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
