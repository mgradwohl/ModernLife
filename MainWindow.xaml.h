// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "Board.h"
#include "fpscounter.h"
#include "TimerHelper.h"

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
        void OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, IInspectable const&);
        unsigned int SetThreadCount() noexcept;

        void OnPropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& args);
        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value)
        {
            return _propertyChanged.add(value);
        }

        void PropertyChanged(winrt::event_token const& token)
        {
            _propertyChanged.remove(token);
        }
        uint16_t SeedPercent() const noexcept;
        void SeedPercent(uint16_t value);
        uint16_t MaxAge() const noexcept;
        void MaxAge(uint16_t value);
        bool ShowLegend() const noexcept;
        void ShowLegend(bool value);
        void BoardWidth(uint16_t value);
        uint16_t BoardWidth() const noexcept;
        hstring GetRandPercentText(double_t value);
        hstring GetBoardWidthText(double_t value);
        void speedClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void theCanvas_SizeChanged(IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
        void GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ruleClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void BuildSpriteSheet(const Microsoft::Graphics::Canvas::CanvasDevice& device);
        const Windows::Foundation::Rect GetSpriteCell(uint16_t index) const noexcept;

        void CanvasControl_Draw(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
        void RenderOffscreen(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender);
        void DrawHorizontalRows(const Microsoft::Graphics::Canvas::CanvasDrawingSession& ds, uint16_t startRow, uint16_t endRow);
        void theCanvasStatsContent_Draw(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args);
        void SetupRenderTargets();
        void theCanvas_CreateResources(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args);
        void SetBestCanvasandWindowSizes();
        void OnBoardResized();
        void RandomizeBoard();
        void OnMaxAgeChanged();

        Windows::UI::Color GetCellColorHSV(uint16_t age);
        Windows::UI::Color GetOutlineColorHSV(uint16_t age);
        Windows::UI::Color HSVtoColor(float h, float s, float v);

    private:
        FPScounter fps{ nullptr };
        TimerHelper timer{ 30, true };

        int _threadcount{ 0 };
        std::vector<Microsoft::Graphics::Canvas::CanvasRenderTarget> _backbuffers;
        std::vector<Microsoft::Graphics::Canvas::CanvasDrawingSession> _dsList;
        Microsoft::Graphics::Canvas::CanvasRenderTarget _spritesheet{ nullptr };

        std::mutex lockbackbuffer;
        std::mutex lockboard;
        Board _board{ nullptr };

        float _dpi{ 0.0f };
        float _dipsPerCellDimension{ 0.0f };
        uint16_t _rowsPerSlice{ 0 };
        float _sliceHeight{ 0.0f };
        uint16_t _spritesPerRow{ 0 };
        float _spriteDipsPerRow{ 0.0f };

        bool _drawLegend{ false };
        uint16_t _randompercent{30};
        uint16_t _maxage{ 1000 };
        int32_t _ruleset{ 1 };
        int16_t _boardwidth{ 200 };
        float _bestcanvassize{ 1000.0f };
        // ensure at least 6 dips per cell max board size which is 500 cells per row
        float _idealbackbuffersize{ 4000.0f };
        float _bestbackbuffersize{ 4000.0f };

        winrt::event_token _propertyToken;
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> _propertyChanged;
    };
}

namespace winrt::ModernLife::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}