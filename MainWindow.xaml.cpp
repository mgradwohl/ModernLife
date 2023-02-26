// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#undef GetCurrentTime

#include <string>

#include <algorithm>
#include <WinUser.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include "microsoft.ui.xaml.window.h"
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.Text.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <Shobjidl.h>
#include "Log.h"
#include "Shape.h"
#include "Renderer.h"
#include "TimerHelper.h"
#include "fpscounter.h"
#include "HSVColorHelper.h"

using namespace winrt;
namespace winrt::ModernLife::implementation
{
    void MainWindow::InitializeComponent()
    {
        Util::Log::Init();
        ML_METHOD;
        ML_INFO("Log Initialized");

        //https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        MainWindowT::InitializeComponent();

        PropertyChanged({ this, &MainWindow::OnPropertyChanged });

        SetMyTitleBar();
        timer.Tick({ this, &MainWindow::OnTick });
        OnFirstRun();
        StartGameLoop();
    }

    void MainWindow::OnFirstRun()
    {
        ML_METHOD;

        //initializes _dpi
        _dpi = gsl::narrow_cast<float>(GetDpiForWindow(GetWindowHandle()));
        const float dpi2 = canvasBoard().Dpi();

        if (dpi2 < _dpi)
        {
            canvasBoard().DpiScale(_dpi / dpi2);
        }
        const float dpi3 = canvasBoard().Dpi();
        if (_dpi != dpi3)
        {
            __debugbreak();
		}

        // initializes _canvasDevice and renderer
        _canvasDevice = Microsoft::Graphics::Canvas::CanvasDevice::GetSharedDevice();
        _renderer.Attach(_canvasDevice, _dpi, MaxAge());

        // initializes _canvasSize and _windowSize
        SetBestCanvasandWindowSizes();

        // initialize the board & renderer
        _board.Reserve(gsl::narrow_cast<size_t>(sliderBoardWidth().Maximum() * sliderBoardWidth().Maximum()));
        OnBoardResized();
    }

    void MainWindow::Pause()
    {
        timer.Stop();
        GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Play));
        GoButton().Label(L"Play");
    }

    void MainWindow::Play()
    {
        timer.Start();
        GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Play));
        GoButton().Label(L"Pause");
    }

    void MainWindow::StartGameLoop()
    {
        ML_METHOD;

        // prep the play button
        Pause();

        // start the FPSCounter
        fps.Start();

        // draw the initial population
        InvalidateIfNeeded();
    }

    void MainWindow::OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, IInspectable const&)
    {
        _board.Update(_ruleset);
        canvasBoard().Invalidate();
        canvasStats().Invalidate();
    }

    void MainWindow::InvalidateIfNeeded()
    {
        if (!timer.IsRunning())
        {
            canvasBoard().Invalidate();
            canvasStats().Invalidate();
        }
    }

    Windows::Foundation::IAsyncOperation<winrt::hstring> MainWindow::PickShapeFileAsync()
    {
        ML_METHOD;

        Windows::Storage::Pickers::FileOpenPicker openPicker;
        auto initializeWithWindow{ openPicker.as<::IInitializeWithWindow>() };
        initializeWithWindow->Initialize(GetWindowHandle());
        openPicker.ViewMode(Windows::Storage::Pickers::PickerViewMode::List);
        openPicker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::Desktop);
        openPicker.FileTypeFilter().ReplaceAll({ L".cells" });
        Windows::Storage::StorageFile sfile = co_await openPicker.PickSingleFileAsync();
        if (sfile == nullptr)
        {
            ML_TRACE("File failed to open or file picker canceled.");
            co_return winrt::hstring(L"");
        }
        co_return sfile.Path();
    }

    winrt::fire_and_forget MainWindow::LoadShape_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ML_METHOD; 

        // keep the current board playing in the background while the user picks a file
        auto filepicker = co_await PickShapeFileAsync();
        std::string file = winrt::to_string(filepicker);

        // load the shape
        Shape shape(file);
        shape.Load();

        const uint16_t maxsize = gsl::narrow_cast<uint16_t>(sliderBoardWidth().Maximum());
        // if it's too big, bail
        if (shape.MaxDimension() > maxsize)
        {
            ML_TRACE("Board is too small for shape");
            co_return;
        }
        Pause();
        RandomPercent(0);

        // Resize the board to ensure the shape fits
        // try to leave space around it
        uint16_t size = shape.MaxDimension() * 2;

        if (size > maxsize)
        {
            // if the shape + padding is too big, just max out the board size
            size = maxsize;
        }
        BoardWidth(size);

        // Copy the shape to the board
        const uint16_t startX = (_board.Width() - shape.Width()) / 2;
        const uint16_t startY = (_board.Height() - shape.Height()) / 2;
        _board.CopyShape(shape, startX, startY);

        InvalidateIfNeeded();
        co_return;
    }

    void MainWindow::OnPointerPressed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        if (sender != canvasBoard())
        {
            return;
        }

        if (e.GetCurrentPoint(canvasBoard().as<Microsoft::UI::Xaml::UIElement>()).Properties().IsLeftButtonPressed())
        {
            _PointerMode = PointerMode::Left;
        }
        else if (e.GetCurrentPoint(canvasBoard().as<Microsoft::UI::Xaml::UIElement>()).Properties().IsRightButtonPressed())
        {
			_PointerMode = PointerMode::Right;
		}
        else
        {
			_PointerMode = PointerMode::None;
		}

    }

    void MainWindow::OnPointerMoved([[maybe_unused]] winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        if (_PointerMode == PointerMode::None || _PointerMode == PointerMode::Middle)
        {
			return;
		}

        bool on = (_PointerMode == PointerMode::Left);

        for (const Microsoft::UI::Input::PointerPoint& point : e.GetIntermediatePoints(canvasBoard().as<Microsoft::UI::Xaml::UIElement>()))
        {
            const GridPoint g = _renderer.GetCellAtPoint(point.Position());

            //ML_TRACE("Point {},{} Cell grid {},{}", point.Position().X, point.Position().Y, g.x, g.y);
            _board.TurnCellOn(g, on);
        }
        InvalidateIfNeeded();
    }

    void MainWindow::OnPointerReleased([[maybe_unused]] winrt::Windows::Foundation::IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e) noexcept
    {
        _PointerMode = PointerMode::None;
    }
    
    void MainWindow::OnPointerExited([[maybe_unused]] winrt::Windows::Foundation::IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e) noexcept
    {
        _PointerMode = PointerMode::None;
    }

    void MainWindow::SetBestCanvasandWindowSizes()
    {
        ML_METHOD;

        const Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(GetWindowHandle());

        // get the window size
        Microsoft::UI::Windowing::DisplayArea displayAreaFallback(nullptr);
        Microsoft::UI::Windowing::DisplayArea displayArea = Microsoft::UI::Windowing::DisplayArea::GetFromWindowId(idWnd, Microsoft::UI::Windowing::DisplayAreaFallback::Nearest);
        const Windows::Graphics::RectInt32 rez = displayArea.OuterBounds();

        // have the renderer figure out the best canvas size, which initializes CanvasSize
        // TODO on WindowResize should call the below
        _renderer.FindBestCanvasSize(rez.Height);

        // setup offsets for sensible default window size
        constexpr int border = 20; // from XAML TODO can we call 'measure' and just retrieve the border width?
        constexpr int stackpanelwidth = 200; // from XAML TODO can we call 'measure' and just retrieve the stackpanel width?

        // ResizeClient wants pixels, not DIPs
        const int wndWidth = gsl::narrow_cast<int>((_renderer.CanvasSize() + stackpanelwidth + border) * _dpi / 96.0f);
        const int wndHeight = gsl::narrow_cast<int>((_renderer.CanvasSize() + border) * _dpi / 96.0f);

        // resize the window
        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            appWnd.ResizeClient(Windows::Graphics::SizeInt32{ wndWidth, wndHeight });
        }
    }

    void MainWindow::CanvasBoard_Draw([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl  const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _renderer.CanvasWidth(), _renderer.CanvasHeight() };
        if (ShowLegend())
        {
            // TODO don't stretch or shrink the _spritesheet if we don't need to
            args.DrawingSession().DrawImage(_renderer.SpriteSheet(), destRect);
        }
        else
        {
            _renderer.Render(args.DrawingSession(), _board);
        }
        fps.AddFrame();
    }

    void MainWindow::CanvasStats_Draw([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        // ok to do this in function local scope
        using namespace Microsoft::UI::Xaml::Controls;
        using namespace Microsoft::UI::Xaml::Media;

        // copy colors, font details etc from other controls to make this canvas visually consistent with the rest of the app
        Microsoft::Graphics::Canvas::Text::CanvasTextFormat canvasFmt{};
        canvasFmt.FontFamily(PaneHeader().FontFamily().Source());
        canvasFmt.FontSize(gsl::narrow_cast<float>(PaneHeader().FontSize()));

        const Windows::UI::Color colorBack{ splitView().PaneBackground().as<SolidColorBrush>().Color() };
        const Windows::UI::Color colorText{ PaneHeader().Foreground().as<SolidColorBrush>().Color() };

        args.DrawingSession().Clear(colorBack);

        // create the strings to draw
        std::wstring strTitle{ L"FPS\r\nGeneration\r\nAlive\r\nTotal Cells\r\n\r\nDPI\r\nCanvas Size\r\nBackbuffer Size\r\nCell Size\r\nThreads" };
        std::wstring strContent = std::format(L"{}:{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:8L}\r\n\r\n{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:.2f}\r\n{:8L}",
            timer.FPS(), fps.FPS(), _board.Generation(), _board.GetLiveCount(), _board.Size(),
            _dpi, _renderer.CanvasSize(), _renderer.BackbufferSize(), _renderer.DipsPerCell(), _renderer.ThreadCount());

        // draw the text left aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Left);
        args.DrawingSession().DrawText(strTitle, 0, 0, 160, 100, colorText, canvasFmt);

        // draw the values right aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Right);
        args.DrawingSession().DrawText(strContent, 0, 0, 160, 100, colorText, canvasFmt);
        args.DrawingSession().Flush();
    }

    // property & event handlers
    void MainWindow::GoButton_Click([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (timer.IsRunning())
        {
            Pause();
        }
        else
        {
            Play();
        }
    }

    void MainWindow::OnRandomizeBoard()
    {
        RandomizeBoard();
        InvalidateIfNeeded();

        StartGameLoop();
    }

    void MainWindow::RandomizeBoard()
    {
        ML_METHOD;

        _board.RandomizeBoard(RandomPercent() / 100.0f, MaxAge());
    }

    void MainWindow::OnCanvasDeviceChanged()
    {
        ML_METHOD;

        _canvasDevice = Microsoft::Graphics::Canvas::CanvasDevice::GetSharedDevice();
        OnDPIChanged();
        SetBestCanvasandWindowSizes();
        _renderer.Device(_canvasDevice);
        InvalidateIfNeeded();
    }

    void MainWindow::OnDPIChanged()
    {
        ML_METHOD;

        const auto dpi = gsl::narrow_cast<float>(GetDpiForWindow(GetWindowHandle()));
        if (_dpi != dpi)
        {
            _dpi = dpi;
            _renderer.Dpi(_dpi);
            // TODO if dpi changes there's a lot of work to do in the renderer
        }
    }

    void MainWindow::OnBoardResized()
    {
        ML_METHOD;

        // create the board, lock it in the case that OnTick is updating it
        // we lock it because changing board parameters will call StartGameLoop()
        Pause();

        _board.Resize(BoardWidth(), BoardHeight(), MaxAge());
        _renderer.Size(BoardWidth(), BoardHeight());

        RandomizeBoard();
        InvalidateIfNeeded();

        StartGameLoop();
    }

    void MainWindow::OnMaxAgeChanged()
    {
        _board.MaxAge(MaxAge());
        _renderer.SpriteMaxIndex(MaxAge());

        InvalidateIfNeeded();
    }

    // boilerplate and standard Windows stuff below
    void MainWindow::OnPropertyChanged([[maybe_unused]] IInspectable const& sender, PropertyChangedEventArgs const& args)
    {
        if (args.PropertyName() == L"DPIChanged")
        {
            OnDPIChanged();
        }

        if (args.PropertyName() == L"MaxAge")
        {
            OnMaxAgeChanged();
        }

        if (args.PropertyName() == L"BoardWidth")
        {
            OnBoardResized();
        }

        if (args.PropertyName() == L"ShowLegend")
        {
            InvalidateIfNeeded();
        }

        if (args.PropertyName() == L"NewCanvasDevice")
        {
            OnCanvasDeviceChanged();
        }

        if (args.PropertyName() == L"RandomPercent")
        {
            OnRandomizeBoard();
        }

        if (args.PropertyName() == L"FirstTime")
        {
            //OnFirstRun();
        }
    }

    void MainWindow::CanvasBoard_CreateResources([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, [[maybe_unused]] Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args)
    {
        ML_METHOD;

        // TODO might want to do the code in the if-block in all cases (for all args.Reason()s
        if (args.Reason() == Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason::DpiChanged)
        {
            _propertyChanged(*this, PropertyChangedEventArgs{ L"DPIChanged" });
        }

        if (args.Reason() == Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason::NewDevice)
        {
            _propertyChanged(*this, PropertyChangedEventArgs{ L"NewCanvasDevice" });
        }

        if (args.Reason() == Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason::FirstTime)
        {
            _propertyChanged(*this, PropertyChangedEventArgs{ L"FirstTime" });
        }
    }

    uint16_t MainWindow::MaxAge() const noexcept
    {
        return _maxage;
    }

    void MainWindow::MaxAge(uint16_t value)
    {
        if (_maxage != value)
        {
            _maxage = value;
            _propertyChanged(*this, PropertyChangedEventArgs{ L"MaxAge" });
        }
    }

    [[nodiscard]] bool MainWindow::ShowLegend() const noexcept
    {
        return _drawLegend;
    }

    void MainWindow::ShowLegend(bool value)
    {
        if (_drawLegend != value)
        {
            _drawLegend = value;
            _propertyChanged(*this, PropertyChangedEventArgs{ L"ShowLegend" });
        }
    }

    [[nodiscard]] uint16_t MainWindow::RandomPercent() const noexcept
    {
        return _randompercent;
    }

    void MainWindow::RandomPercent(uint16_t value)
    {
        if (_randompercent != value)
        {
            _randompercent = value;
            _propertyChanged(*this, PropertyChangedEventArgs{ L"RandomPercent" });
        }
    }

    [[nodiscard]] uint16_t MainWindow::BoardWidth() const noexcept
    {
        return _boardwidth;
    }

    [[nodiscard]] uint16_t MainWindow::BoardHeight() const noexcept
    {
        return _boardheight;
    }

    void MainWindow::BoardWidth(uint16_t value)
    {
        if (_boardwidth != value)
        {
            _boardwidth = value;
            _boardheight = value;
            _propertyChanged(*this, PropertyChangedEventArgs{ L"BoardWidth" });
        }
    }

    void MainWindow::RandomizeButton_Click([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        _propertyChanged(*this, PropertyChangedEventArgs{ L"RandomPercent" });
    }

    void MainWindow::CanvasBoard_SizeChanged([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        _renderer.Size(BoardWidth(), BoardHeight());
        // might need to BuildSpriteSheet here
        // TODO test
    }

    [[nodiscard]] hstring MainWindow::GetRandomPercentText(double_t value) const
    {
        std::wstring text = std::format(L"{}% random", gsl::narrow_cast<int>(value));
        hstring htext{ text };
        return htext;
    }

    [[nodiscard]] hstring MainWindow::GetBoardWidthText(double_t value) const
    {
        std::wstring text = std::format(L"Width {0} x Height {0}", gsl::narrow_cast<int>(value));
        hstring htext{ text };
        return htext;
    }

    void MainWindow::speedClick(IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        Microsoft::UI::Xaml::Controls::MenuFlyoutItem item = sender.as<Microsoft::UI::Xaml::Controls::MenuFlyoutItem>();
		dropdownSpeed().Content(winrt::box_value(item.Text()));

		timer.FPS(item.Tag().as<int>());
    }

    void MainWindow::ruleClick(IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        Microsoft::UI::Xaml::Controls::MenuFlyoutItem item = sender.as<Microsoft::UI::Xaml::Controls::MenuFlyoutItem>();
        dropdownRules().Content(winrt::box_value(item.Text()));

        _ruleset = item.Tag().as<int>();
    }

    void MainWindow::SetMyTitleBar()
    {
        // Set window title
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());

        const Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(GetWindowHandle());
        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            appWnd.Title(L"ModernLife");

#ifdef _DEBUG
            AppTitlePreview().Text(L"PREVIEW DEBUG");
#endif

            // position near the top of the screen only on launch
            Windows::Graphics::PointInt32 pos{ appWnd.Position() };
            pos.Y = 20;
            appWnd.Move(pos);
        }
    }

    [[nodiscard]] HWND MainWindow::GetWindowHandle() const
    {
        // get window handle, window ID
        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ nullptr };
        winrt::check_hresult(windowNative->get_WindowHandle(&hWnd));
        return hWnd;
    }

    void MainWindow::OnWindowActivate([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] WindowActivatedEventArgs const& args)
    {
        if (args.WindowActivationState() == Microsoft::UI::Xaml::WindowActivationState::Deactivated)
        {
            Microsoft::UI::Xaml::Media::SolidColorBrush brush = Microsoft::UI::Xaml::ResourceDictionary().Lookup(winrt::box_value(L"WindowCaptionForegroundDisabled")).as<Microsoft::UI::Xaml::Media::SolidColorBrush>();
            AppTitleTextBlock().Foreground(brush);
            AppTitlePreview().Foreground(brush);
        }
        else
        {
            Microsoft::UI::Xaml::Media::SolidColorBrush brush = Microsoft::UI::Xaml::ResourceDictionary().Lookup(winrt::box_value(L"WindowCaptionForeground")).as<Microsoft::UI::Xaml::Media::SolidColorBrush>();
            AppTitleTextBlock().Foreground(brush);
            AppTitlePreview().Foreground(brush);
        }
    }

    void MainWindow::OnWindowClosed([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept
    {
        ML_METHOD;
        Util::Log::Shutdown();
        //PropertyChangedRevoker();
    }

    void MainWindow::OnWindowResized([[maybe_unused]] Windows::Foundation::IInspectable const& sender, [[maybe_unused]] Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& args) noexcept
    {
        // TODO lots to do here
    }
}