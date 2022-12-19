// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "fpscounter.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::ModernLife::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        #ifdef _DEBUG
                AppTitlePreview().Text(L"PREVIEW DEBUG");
        #endif
    
        auto windowNative{ this->try_as<::IWindowNative>() };
        winrt::check_bool(windowNative);
        HWND hWnd{ nullptr };
        windowNative->get_WindowHandle(&hWnd);
        Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(hWnd);

        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            Windows::Graphics::PointInt32 pos(appWnd.Position());
            pos.Y = 32;
            appWnd.Move(pos);
            appWnd.ResizeClient(Windows::Graphics::SizeInt32{2220, 1920});
            auto presenter = appWnd.Presenter().as<Microsoft::UI::Windowing::OverlappedPresenter>();
            //presenter.IsMaximizable(false);
            //presenter.IsResizable(false);
        }

        StartGameLoop();
    }

    void MainWindow::InitializeAssets()
    {
        // this will be used to iterate through the width and height of the rendertarget *without* using the partial tile
        uint16_t assetStride = static_cast<uint16_t>(std::sqrt(maxage) + 1);

        // create a square render target that will hold all the tiles this may often times have a partial 'tile' at the end which we won't use
        float rtsize = _widthCellDest * assetStride;

        // if the back buffer doesn't exist or is the wrong size, create it
        CanvasDevice device = CanvasDevice::GetSharedDevice();
        {
            _assets = CanvasRenderTarget(device, rtsize, rtsize, theCanvas().Dpi());
        }

        CanvasDrawingSession ds = _assets.CreateDrawingSession();
        ds.FillRectangle(0, 0, rtsize, rtsize, Colors::WhiteSmoke());

        float posx = 0.0f;
        float posy = 0.0f;

        // start filling tiles at age 0
        uint16_t index = 0;
        for (uint16_t y = 0; y < assetStride; y++)
        {
            for (uint16_t x = 0; x < assetStride; x++)
            {
                //ds.DrawRectangle(posx, posy, w, w, GetCellColor3(cell));
                //ds.DrawRoundedRectangle(posx, posy, w, w, 2, 2, GetCellColorHSV(index));
                ds.FillRoundedRectangle(posx, posy, _widthCellDest, _widthCellDest, 2, 2, GetCellColorHSV(index));
                posx += _widthCellDest;
                index++;
            }
            posy += _widthCellDest;
            posx = 0.0f;
        }
    }

    void MainWindow::OnWindowActivate(IInspectable const& sender, WindowActivatedEventArgs const& args)
    {
        sender;
        using namespace Microsoft::UI::Xaml::Media;

        if (args.WindowActivationState() == WindowActivationState::Deactivated)
        {
            SolidColorBrush brush = ResourceDictionary().Lookup(winrt::box_value(L"WindowCaptionForegroundDisabled")).as<SolidColorBrush>();
            AppTitleTextBlock().Foreground(brush);
            AppTitlePreview().Foreground(brush);
        }
        else
        {
            SolidColorBrush brush = ResourceDictionary().Lookup(winrt::box_value(L"WindowCaptionForeground")).as<SolidColorBrush>();
            AppTitleTextBlock().Foreground(brush);
            AppTitlePreview().Foreground(brush);
        }
    }

    void MainWindow::OnTick(IInspectable const& sender, IInspectable const& event)
    {
        sender;
        event;

        {
            std::scoped_lock lock{ lockboard };
            board.ConwayUpdateBoardWithNextState();
            board.ApplyNextStateToBoard();
        }
        theCanvas().Invalidate();
    }

    void MainWindow::StartGameLoop()
    {
        {
            // create and start a timer without recreating it when the user changes options
            if (nullptr == _controller)
            {
                _controller = DispatcherQueueController::CreateOnDedicatedThread();
            }

            if (nullptr == _queue)
            {
                _queue = _controller.DispatcherQueue();
            }

            if (nullptr == _timer)
            {
                _timer = _queue.CreateTimer();
            }

            if (! _tokeninit)
            {
                _registrationtoken = _timer.Tick({ this, &MainWindow::OnTick });
                _tokeninit = true;
            }

            using namespace  std::literals::chrono_literals;

            _timer.Interval(std::chrono::milliseconds(1000/_speed));
            _timer.IsRepeating(true);
        }

        using namespace Microsoft::UI::Xaml::Controls;
        GoButton().Icon(SymbolIcon(Symbol::Play));
        GoButton().Label(L"Play");

        // create the board, lock it in the case that OnTick is updating it
        {
            std::scoped_lock lock{ lockboard };
            board = Board{ static_cast<uint16_t>(_boardwidth), static_cast<uint16_t>(_boardwidth) };
        }

        // add a random population
        auto randomizer = std::async(&Board::RandomizeBoard, &board, _randompercent / 100.0f);
        randomizer.wait();

        // start the FPSCounter
        fps.Start();
        
        // draw the initial population
        theCanvas().Invalidate();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        {
            std::scoped_lock lock{ lockbackbuffer };
            args.DrawingSession().DrawImage(_back, 0, 0);
            //args.DrawingSession().DrawImage(_assets, 0, 0);
        }
        fps.AddFrame();
    }

    Windows::UI::Color MainWindow::GetCellColorHSV(uint16_t age)
    {
        if (age > maxage)
        {
            return Windows::UI::Colors::Black();
        }

        float h = (age * 360.f) / maxage;
        return HSVtoColor(h, 0.5f, 0.8f);
    }

    void MainWindow::DrawInto(CanvasDrawingSession& ds, uint16_t startY, uint16_t endY)
	{
        //float inc = width / cellcount;
        //if (drawgrid)
		//{
		//	for (int i = 0; i <= cellcount; i++)
		//	{
		//		ds.DrawLine(0, i * inc, height, i * inc, Colors::DarkSlateGray());
		//		ds.DrawLine(i * inc, 0, i * inc, width, Colors::DarkSlateGray());
		//	}
		//}

        float srcW = _widthCellDest;
        uint16_t srcStride = static_cast<uint16_t>(std::sqrt(maxage) + 1);

        //auto spriteBatch = ds.CreateSpriteBatch();

        float posx = 0.0f;
		float posy = startY * _widthCellDest;
        {
            for (uint16_t y = startY; y < endY; y++)
            {
                for (uint16_t x = 0; x < board.Width(); x++)
                {
                    if (const Cell& cell = board.GetCell(x, y); cell.IsAlive())
                    {
                        float srcX = static_cast<float>(cell.Age() % srcStride);
                        float srcY = static_cast<float>(cell.Age() / srcStride);
                        Windows::Foundation::Rect srcRect{ srcW * srcX, srcW * srcY, srcW, srcW};
                        Windows::Foundation::Rect srcDest{ posx, posy, _widthCellDest, _widthCellDest};

                        //ds.DrawRoundedRectangle(posx, posy, w, w, 2, 2, GetCellColorHSV(cell.Age()));
                        ds.FillRoundedRectangle(posx, posy, _widthCellDest, _widthCellDest, 2, 2, GetCellColorHSV(cell.Age()));

                        // this is not actually faster - unexpected
                        // and also has some drawing artifacts (black goes to pink and back to black) and missing rounded edges
                        //spriteBatch.DrawFromSpriteSheet(_assets, srcDest, srcRect);
                        //ds.DrawImage(_assets, srcDest, srcRect);
                    }
                    posx += _widthCellDest;
                }
                posy += _widthCellDest;
                posx = 0.0f;
            }
        }
	}

    void MainWindow::SetupRenderTargets()
    {
        CanvasDevice device = CanvasDevice::GetSharedDevice();

        {
            std::scoped_lock lock{ lockbackbuffer };
            _back = CanvasRenderTarget(device, _canvasSize, _canvasSize, theCanvas().Dpi());
            _widthCellDest = (_canvasSize / _boardwidth);
        }
        InitializeAssets();
    }

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

        if (!board.IsDirty())
            return;

        CanvasDrawingSession ds = _back.CreateDrawingSession();
        ds.FillRectangle(0, 0, _canvasSize, _canvasSize, Colors::WhiteSmoke());

        if (board.GetSize() < 100000)
        {
            // render in one thread
            std::scoped_lock lock{ lockboard };
            auto drawinto0 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(0), static_cast<uint16_t>(board.Height()));
            drawinto0.wait();
        }
        else if (board.GetSize() >= 100000 && board.GetSize() < 200000)
        {
            std::scoped_lock lock{ lockboard };

            // render in 4 threads
            auto drawinto1 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(0), static_cast<uint16_t>(board.Height() * 1 / 4));
            auto drawinto2 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 1 / 4), static_cast<uint16_t>(board.Height() * 1 / 2));
            auto drawinto3 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 1 / 2), static_cast<uint16_t>(board.Height() * 3 / 4));
            auto drawinto4 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 3 / 4), static_cast<uint16_t>(board.Height()));

            drawinto1.wait();
            drawinto2.wait();
            drawinto3.wait();
            drawinto4.wait();
        }
        else
        {
            {
                std::scoped_lock lock{ lockboard };
                // render in 8 threads
                auto drawinto1 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(0), static_cast<uint16_t>(board.Height() * 1 / 8));
                auto drawinto2 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 1 / 8), static_cast<uint16_t>(board.Height() * 2 / 8));
                auto drawinto3 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 2 / 8), static_cast<uint16_t>(board.Height() * 3 / 8));
                auto drawinto4 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 3 / 8), static_cast<uint16_t>(board.Height() * 4 / 8));
                auto drawinto5 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 4 / 8), static_cast<uint16_t>(board.Height() * 5 / 8));
                auto drawinto6 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 5 / 8), static_cast<uint16_t>(board.Height() * 6 / 8));
                auto drawinto7 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 6 / 8), static_cast<uint16_t>(board.Height() * 7 / 8));
                auto drawinto8 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 7 / 8), static_cast<uint16_t>(board.Height()));

                drawinto1.wait();
                drawinto2.wait();
                drawinto3.wait();
                drawinto4.wait();
                drawinto5.wait();
                drawinto6.wait();
                drawinto7.wait();
                drawinto8.wait();
            }
        }
//        ds.Flush();
    }

    int32_t MainWindow::SeedPercent() const
    {
        return _randompercent;
    }

    void MainWindow::SeedPercent(int32_t value)
    {
        if (_randompercent != value)
        {
            _randompercent = value;

            m_propertyChanged(*this, PropertyChangedEventArgs{ L"SeedPercent" });
        }
    }

    int16_t MainWindow::BoardWidth() const
    {
        return _boardwidth;
    }

    void MainWindow::BoardWidth(int16_t value)
    {
        if (_boardwidth != value)
        {
            _boardwidth = value;
            _timer.Stop();
            m_propertyChanged(*this, PropertyChangedEventArgs{ L"BoardWidth" });
            SetupRenderTargets();

            StartGameLoop();
        }
    }
    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::theCanvasStatsContent_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        sender;
        
        using namespace Microsoft::UI::Xaml::Controls;
        using namespace Microsoft::UI::Xaml::Media;

        // copy colors, font details etc from other controls
        // to make this canvas aligned with styles
        Microsoft::Graphics::Canvas::Text::CanvasTextFormat canvasFmt{};
        canvasFmt.FontFamily(PaneHeader().FontFamily().Source());
        canvasFmt.FontSize(static_cast<float>(PaneHeader().FontSize()));

        Brush backBrush{ splitView().PaneBackground() };
        Brush textBrush{ PaneHeader().Foreground() };

        SolidColorBrush scbBack = backBrush.try_as<SolidColorBrush>();
        SolidColorBrush scbText = textBrush.try_as<SolidColorBrush>();

        Windows::UI::Color colorBack{scbBack.Color()};
        Windows::UI::Color colorText{scbText.Color()};

        args.DrawingSession().Clear(colorBack);

        // create the strings to draw
        std::wstring strTitle = std::format(L"FPS\r\nGeneration\r\nAlive\r\nTotal Cells");
        std::wstring strContent = std::format(L"{}:{:.1f}\r\n{:8}\r\n{:8}\r\n{:8}", _speed, fps.FPS(), board.Generation(), board.GetLiveCount(), board.GetSize());
        sender.Invalidate();
        
        // draw the text left aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Left);
        args.DrawingSession().DrawText(strTitle, 0, 0, 160, 100, colorText, canvasFmt);

        // draw the values right aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Right);
        args.DrawingSession().DrawText(strContent, 0, 0, 160, 100, colorText, canvasFmt);
//        args.DrawingSession().Flush();
    }

    void MainWindow::GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        sender;

        using namespace Microsoft::UI::Xaml::Controls;
        if (_timer.IsRunning())
        {
            GoButton().Label(L"Play");
            GoButton().Icon(SymbolIcon(Symbol::Play));
            _timer.Stop();

        }
        else
        {
            GoButton().Icon(SymbolIcon(Symbol::Pause));
            GoButton().Label(L"Pause");
            _timer.Start();
        }
    }

    void MainWindow::RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        sender;

        using namespace Microsoft::UI::Xaml::Controls;

        _timer.Stop();
        GoButton().Icon(SymbolIcon(Symbol::Pause));
        GoButton().Label(L"Pause");
        StartGameLoop();
    }

    void MainWindow::theCanvas_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        _canvasSize = min(e.NewSize().Width, bestsize);
        SetupRenderTargets();
    }

    hstring MainWindow::GetRandPercentText(double_t value)
    {
        std::wstring text = std::format(L"{0}% random", static_cast<int>(value));
        hstring htext{ text };
        return htext;
    }

    hstring MainWindow::GetBoardWidthText(double_t value)
    {
        std::wstring text = std::format(L"Width {0} x Height {0}", static_cast<int>(value));
        hstring htext{ text };
        return htext;
    }

    void MainWindow::speedClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        using namespace Microsoft::UI::Xaml::Controls;
        MenuFlyoutItem item = sender.try_as<MenuFlyoutItem>();
        _speed = item.Tag().as<int>();
        
		dropdownSpeed().Content(winrt::box_value(item.Text()));

		_timer.Interval(std::chrono::milliseconds(1000/_speed));
    }

    // Adapted from https://www.cs.rit.edu/~ncs/color/t_convert.html#RGB%20to%20XYZ%20&%20XYZ%20to%20RGB
    Windows::UI::Color MainWindow::HSVtoColor(float h, float s, float v)
    {
        if (h > 360.0f)
        {
            __debugbreak();
        }
        if (s == 0)
        {
            uint8_t x = static_cast<uint8_t>(v);
            return ColorHelper::FromArgb(255, x, x, x);
        }
        
        h /= 60;
        int i = static_cast<int>(std::floor(h));
        float f = h - i;
        float p = v * (1 - s);
        float q = v * (1 - s * f);
        float t = v * (1 - s * (1 - f));

        float dr = 0;
        float dg = 0;
        float db = 0;

        switch (i)
        {
            case 0:
                dr = v;
                dg = t;
                db = p;
                break;
            case 1:
                dr = q;
                dg = v;
                db = p;
                break;
            case 2:
                dr = p;
                dg = v;
                db = t;
                break;
            case 3:
                dr = p;
                dg = q;
                db = v;
                break;
            case 4:
                dr = t;
                dg = p;
                db = v;
                break;
            case 5:
                dr = v;
                dg = p;
                db = q;
                break;
            default:
                dr = v;
                dg = v;
                db = v;
                break;
        }

        uint8_t r = static_cast<uint8_t>(dr * 255);
        uint8_t g = static_cast<uint8_t>(dg * 255);
        uint8_t b = static_cast<uint8_t>(db * 255);

        return ColorHelper::FromArgb(255, r, g, b);
    }
}
