#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#undef GetCurrentTime

#include <string>
#include <sstream>
#include <algorithm>

#include <WinUser.h>
#include <Shobjidl.h>

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
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.Text.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>

#include <wil/cppwinrt.h>
#include <wil/cppwinrt_helpers.h>

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
        ML_INFO("Log Initialized");
        ML_METHOD;

        //https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        MainWindowT::InitializeComponent();

        winrt::Microsoft::UI::Xaml::Media::MicaBackdrop backdrop;
        SystemBackdrop(backdrop);

        PropertyChanged({ this, &MainWindow::OnPropertyChanged });

        SetMyTitleBar();
        timer.Tick({ get_strong(), &MainWindow::OnTick });
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

        // initialize _canvasDevice
        _canvasDevice = Microsoft::Graphics::Canvas::CanvasDevice::GetSharedDevice();

        // initialize board
        _board.Reserve(gsl::narrow_cast<size_t>(sliderBoardWidth().Maximum() * sliderBoardWidth().Maximum()));
        _board.Resize(BoardWidth(), BoardHeight(), MaxAge());
        RandomizeBoard();

        // intitialize renderer
        _renderer.Attach(_canvasDevice, _dpi, MaxAge());
        _renderer.Size(BoardWidth(), BoardHeight());

        SetBestCanvasandWindowSizes();
    }

    void MainWindow::Pause()
    {
        SetStatus("Paused. Press Play to start. Left mouse button to draw. Right right mouse button to erase.");
        timer.Stop();
        GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Play));
        GoButton().Label(L"Play");
    }

    void MainWindow::Play()
    {
        SetStatus("Running... Left mouse button to draw. Right right mouse button to erase.");
        timer.Start();
        GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Pause));
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
        ML_METHOD;
        canvasBoard().Invalidate();
    }

    void MainWindow::PumpProperties()
    {
        _propertyChanged(*this, PropertyChangedEventArgs{ L"FPSAverage" });
        _propertyChanged(*this, PropertyChangedEventArgs{ L"GenerationCount" });
        _propertyChanged(*this, PropertyChangedEventArgs{ L"LiveCount" });
    }

    void MainWindow::InvalidateIfNeeded()
    {
        if (!timer.IsRunning())
        {
            canvasBoard().Invalidate();
        }
        PumpProperties();
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

    fire_and_forget MainWindow::ShowMessageBox(const hstring& title, const hstring& message)
    {
        winrt::Microsoft::UI::Xaml::Controls::ContentDialog dialog;

        // XamlRoot must be set in the case of a ContentDialog running in a Desktop app
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.Style( Microsoft::UI::Xaml::Application::Current().Resources().TryLookup(winrt::box_value(L"DefaultContentDialogStyle")).as<Microsoft::UI::Xaml::Style>() );
        dialog.Title(winrt::box_value(title));
        dialog.Content(winrt::box_value(message));
        dialog.PrimaryButtonText(L"OK");
        dialog.DefaultButton(winrt::Microsoft::UI::Xaml::Controls::ContentDialogButton::Primary);

        auto result = co_await dialog.ShowAsync();
        result;
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
            hstring title = L"Shape too big";

            std::ostringstream wss;
            wss << shape.Name() << " is too big for ModernLife.\r\n\r\nWidth: " << shape.Width() << " Height: " << shape.Height() << std::endl;
            SetStatus(wss.str());

            hstring wmsg = winrt::to_hstring(wss.str());
            ShowMessageBox(title, wmsg);
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
        SetStatus("Loaded " + shape.Name());

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
            //SetStatus("Drawing. Left mouse button to draw. Right right mouse button to erase.");

            _board.TurnCellOn(g, on);
        }
        InvalidateIfNeeded();
    }

    void MainWindow::OnPointerReleased([[maybe_unused]] winrt::Windows::Foundation::IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e) noexcept
    {
        //SetStatus("Drawing mode completed.");
        _PointerMode = PointerMode::None;
    }
    
    void MainWindow::OnPointerExited([[maybe_unused]] winrt::Windows::Foundation::IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e) noexcept
    {
        //SetStatus("Drawing mode completed.");
        _PointerMode = PointerMode::None;
    }

    void MainWindow::SetBestCanvasandWindowSizes()
    {
        ML_METHOD;
        if (_dpi == 0.0f)
        {
            return;
        }

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
        constexpr int statusheight = 28;

        // ResizeClient wants pixels, not DIPs
        const int wndWidth = gsl::narrow_cast<int>((_renderer.CanvasSize() + stackpanelwidth + border) * _dpi / 96.0f);
        const int wndHeight = gsl::narrow_cast<int>((_renderer.CanvasSize() + border + statusheight) * _dpi / 96.0f);

        // resize the window
        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            appWnd.ResizeClient(Windows::Graphics::SizeInt32{ wndWidth, wndHeight });
        }
    }

    void MainWindow::CanvasBoard_Draw([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl  const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        _board.Update(_ruleset);

        if (ShowLegend())
        {
            // TODO don't stretch or shrink the _spritesheet if we don't need to
            const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _renderer.CanvasWidth(), _renderer.CanvasHeight() };
            args.DrawingSession().DrawImage(_renderer.SpriteSheet(), destRect);
        }
        else
        {
            _renderer.Render(args.DrawingSession(), _board);
        }
        fps.AddFrame();
        PumpProperties();
    }

    void MainWindow::SetStatus(const std::string& message)
    {
        _statusMain = message;
        _propertyChanged(*this, PropertyChangedEventArgs{ L"StatusMain" });
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
        SetStatus("Board reset");

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
        SetStatus("Canvas device changed");

        _canvasDevice = Microsoft::Graphics::Canvas::CanvasDevice::GetSharedDevice();
        OnDPIChanged();
        SetBestCanvasandWindowSizes();
        _renderer.Device(_canvasDevice);
        InvalidateIfNeeded();
    }

    void MainWindow::OnDPIChanged()
    {
        ML_METHOD;
        SetStatus("DPI changed");

        const auto dpi = gsl::narrow_cast<float>(GetDpiForWindow(GetWindowHandle()));
        if (_dpi != dpi)
        {
            ML_TRACE("MainWindow DPI changed from {} to {}", dpi, _dpi);
            _dpi = dpi;
            _renderer.Dpi(_dpi);
            // TODO if dpi changes there's a lot of work to do in the renderer
        }
        else
        {
            ML_TRACE("MainWindow DPI Not Changed {}", _dpi);
        }
    }

    void MainWindow::OnBoardResized()
    {
        ML_METHOD;
        SetStatus("Board resized");
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
            //SetStatus("First run event.");
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

    [[nodiscard]] hstring MainWindow::StatusMain() const
    {
        return winrt::to_hstring(_statusMain);
    }

    [[nodiscard]] hstring MainWindow::LiveCount() const
    {
        return winrt::to_hstring(_board.GetLiveCount());
    }

    [[nodiscard]] hstring MainWindow::GenerationCount() const
    {
        return winrt::to_hstring(_board.Generation());
    }

    [[nodiscard]] hstring MainWindow::FPSAverage() const
    {
        std::string f = std::format("{:.2f}", fps.FPS());
        return winrt::to_hstring(f);
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
        //timer.Revoke();
        Util::Log::Shutdown();
        //PropertyChangedRevoker();
    }

    void MainWindow::OnWindowResized([[maybe_unused]] Windows::Foundation::IInspectable const& sender, [[maybe_unused]] Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& args) noexcept
    {
        ML_METHOD;

        if (_dpi == 0)
        {
            ML_TRACE("Ignoring resize, DPI == 0");
            return;
        }

        SetBestCanvasandWindowSizes();
         _renderer.WindowResize();

        // TODO lots to do here if we let the user resize the window and that resizes the canvas
        // right now the canvas size is fixed
    }
}