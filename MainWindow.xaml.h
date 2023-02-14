﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

#include <windows.h>

#include <string>

#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "Renderer.h"
#include "Board.h"
#include "fpscounter.h"
#include "TimerHelper.h"

namespace winrt::ModernLife::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        // construction and initialization
        MainWindow() = default;
        void InitializeComponent();

        // standard window stuff
        void OnWindowActivate(IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowActivatedEventArgs const& args);
        void SetMyTitleBar();
        void OnWindowClosed(IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept;
        void InvalidateIfNeeded();

        // starting the game and handling the timer
        void StartGameLoop();
        void OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, IInspectable const&);

        // drawing stats
        void CanvasStats_Draw(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);

        // property change handlers
        void OnPropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& args);
        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value)
        {
            return _propertyChanged.add(value);
        }

        void PropertyChanged(winrt::event_token const& token)
        {
            _propertyChanged.remove(token);
        }

        // xaml bindings
        uint16_t RandomPercent() const noexcept;
        void RandomPercent(uint16_t value);
        uint16_t MaxAge() const noexcept;
        void MaxAge(uint16_t value);
        bool ShowLegend() const noexcept;
        void ShowLegend(bool value);
        void BoardWidth(uint16_t value);
        uint16_t BoardWidth() const noexcept;
        uint16_t BoardHeight() const noexcept;
        hstring GetRandomPercentText(double_t value) const;
        hstring GetBoardWidthText(double_t value) const;

        // event handlers
        void speedClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CanvasBoard_SizeChanged(IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
        void GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RandomizeButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ruleClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CanvasBoard_CreateResources(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args);
        void OnRandomizeBoard();
        HWND GetWindowHandle() const;
        void OnDPIChanged();
        void OnCanvasDeviceChanged();
        void SetBestCanvasandWindowSizes();
        void CanvasBoard_Draw(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
        void OnBoardResized();
        void RandomizeBoard();
        void OnMaxAgeChanged();
        void OnFirstRun();

    private:
        Microsoft::Graphics::Canvas::CanvasDevice _canvasDevice{ nullptr };

        Renderer _renderer;
        Board _board;
        FPScounter fps{};
        TimerHelper timer{ 30, true };

        float _dpi{ 0.0f };

        bool _drawLegend{ false };
        uint16_t _randompercent{30};
        uint16_t _maxage{ 1000 };
        int32_t _ruleset{ 1 };
        uint16_t _boardwidth{ 200 };
        uint16_t _boardheight{ 200 };

        winrt::event_token _propertyToken;
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> _propertyChanged;
    public:
        void OnWindowResized(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& args);
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}