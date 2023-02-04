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
        void OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, IInspectable const&);
        unsigned int SetThreadCount() noexcept;

        void OnPropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& args);
        winrt::event_token PropertyChanged(PropertyChangedEventHandler const& value)
        {
            return _propertyChanged.add(value);
        }

        void PropertyChanged(winrt::event_token const& token)
        {
            _propertyChanged.remove(token);
        }
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
        void GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ruleClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void BuildSpriteSheet(const CanvasDevice& device);
        const Windows::Foundation::Rect GetSpriteCell(int index) const noexcept;

        void CanvasControl_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void RenderOffscreen(CanvasControl const& sender);
        void DrawHorizontalRows(const CanvasDrawingSession& ds, int startRow, int endRow);
        void theCanvasStatsContent_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args);
        void SetupRenderTargets();
        void theCanvas_CreateResources(CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args);
        void SetBestCanvasandWindowSizes();
        void OnBoardResized();
        void RandomizeBoard();

        Windows::UI::Color GetCellColorHSV(uint16_t age);
        Windows::UI::Color GetOutlineColorHSV(uint16_t age);
        Windows::UI::Color HSVtoColor(float h, float s, float v);

    private:
        FPScounter fps{ nullptr };
        TimerHelper timer{ 30, true };

        int _threadcount{ 0 };
        std::vector<CanvasRenderTarget> _backbuffers;
        std::vector<CanvasDrawingSession> _dsList;
        CanvasRenderTarget _spritesheet{ nullptr };

        std::mutex lockbackbuffer;
        std::mutex lockboard;
        Board _board{ nullptr };

        float _dpi{ 0.0f };
        float _dipsPerCellDimension{ 0.0f };
        uint16_t _rowsPerSlice{ 0 };
        float _sliceHeight{ 0.0f };
        int _spritesPerRow{ 0 };
        float _spriteDipsPerRow{ 0.0f };

        bool _drawLegend{ false };
        int32_t _randompercent{30};
        int32_t _maxage{ 1000 };
        int32_t _ruleset{ 1 };
        int16_t _boardwidth{ 200 };
        float _bestcanvassize{ 1000.0f };
        // ensure at least 6 dips per cell max board size which is 500 cells per row
        float _idealbackbuffersize{ 4000.0f };
        float _bestbackbuffersize{ 4000.0f };

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