// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.Foundation.h>
#include "TimerHelper.h"
#include "fpscounter.h"

namespace winrt::ModernLife::implementation
{
    MainWindow::MainWindow() noexcept
    {
        //InitializeComponent(); https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
    }

    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
#ifdef _DEBUG
        AppTitlePreview().Text(L"PREVIEW DEBUG");
#endif

        auto windowNative{ this->try_as<::IWindowNative>() };
        winrt::check_bool(windowNative);
        HWND hWnd{ nullptr };
        windowNative->get_WindowHandle(&hWnd);
        const Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(hWnd);

        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            Windows::Graphics::PointInt32 pos{ appWnd.Position() };
            pos.Y = 32;
            appWnd.Move(pos);
            appWnd.ResizeClient(Windows::Graphics::SizeInt32{ 2220, 1920 });
            auto presenter = appWnd.Presenter().as<Microsoft::UI::Windowing::OverlappedPresenter>();
            //presenter.IsMaximizable(false);
            //presenter.IsResizable(false);
        }

        // doesn't work see TimerHelper.h
        //timer = TimerHelper({ this, &MainWindow::OnTick }, 60, true);
        
        timer.Tick({ this, &MainWindow::OnTick });
        StartGameLoop();
    }

    void MainWindow::Window_Closed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept
    {
        sender;
        args;
    }

    void MainWindow::InitializeAssets(const CanvasDevice& device)
    {
        // this will be used to iterate through the width and height of the rendertarget *without* adding a partial tile at the end of a row
        const uint16_t assetStride = gsl::narrow_cast<uint16_t>(std::sqrt(maxage)) + 1;

        // create a square render target that will hold all the tiles (this will avoid a partial 'tile' at the end which we won't use)
        const float rtsize = _widthCellDest * assetStride;

        // if the sprite sheet doesn't exist or is the wrong size, create it
        _assets = CanvasRenderTarget(device, rtsize, rtsize, theCanvas().Dpi());

        CanvasDrawingSession ds = _assets.CreateDrawingSession();
        ds.Clear(Colors::WhiteSmoke());
        ds.Antialiasing(CanvasAntialiasing::Antialiased);

        float posx{ 0.0f };
        float posy{ 0.0f };

        float round{ 2.0f };
        float offset{ 0.25f };
        if (_widthCellDest > 20)
        {
            round = 6.0f;
            offset = 0.5f;
        }

        // start filling tiles at age 0
        uint16_t index = 0;
        for (uint16_t y = 0; y < assetStride; y++)
        {
            for (uint16_t x = 0; x < assetStride; x++)
            {
                ds.FillRoundedRectangle(posx + offset, posy + offset, _widthCellDest - (2 * offset), _widthCellDest - (2 * offset), round, round, GetCellColorHSV(index));
                ds.DrawRoundedRectangle(posx + offset, posy + offset, _widthCellDest - (2 * offset), _widthCellDest - (2 * offset), round, round, GetOutlineColorHSV(index), 1.0f);
                posx += _widthCellDest;
                index++;
            }
            posy += _widthCellDest;
            posx = 0.0f;
        }
        ds.Flush();
        ds.Close();
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

    void MainWindow::OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, winrt::Windows::Foundation::IInspectable const&)
    {
        //sender;
        //event;

        {
            std::scoped_lock lock{ lockboard };
            board.ConwayUpdateBoardWithNextState();
            board.ApplyNextStateToBoard();
        }
        theCanvas().Invalidate();
    }

    void MainWindow::StartGameLoop()
    {
        // prep the play button
        using namespace Microsoft::UI::Xaml::Controls;
        GoButton().Icon(SymbolIcon(Symbol::Play));
        GoButton().Label(L"Play");

        // create the board, lock it in the case that OnTick is updating it
        // we lock it because changing board parameters will call StartGameLoop()
        {
            std::scoped_lock lock{ lockboard };
            board = Board{ gsl::narrow_cast<uint16_t>(_boardwidth), gsl::narrow_cast<uint16_t>(_boardwidth) };
        }

        // add a random population
        auto randomizer = std::async(&Board::RandomizeBoard, &board, _randompercent / 100.0f);
        randomizer.wait();

        // start the FPSCounter
        fps = FPScounter();
        
        // draw the initial population
        theCanvas().Invalidate();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        {
            std::scoped_lock lock{ lockbackbuffer };
            args.DrawingSession().Antialiasing(CanvasAntialiasing::Aliased);
            args.DrawingSession().Blend(CanvasBlend::Copy);

            // comment out the following to see the sprite sheet
            args.DrawingSession().DrawImage(_back);
            
            // uncomment the following line to see the sprite sheet
            //args.DrawingSession().DrawImage(_assets, 0, 0);
            args.DrawingSession().Flush();
        }
        fps.AddFrame();
    }

    Windows::UI::Color MainWindow::GetCellColorHSV(uint16_t age)
    {
        // should never see a black cell
        if (age > maxage)
        {
            return Windows::UI::Colors::Black();
        }

        const float h{ (age * 360.f) / maxage };
        return HSVtoColor(h, 0.6f, 0.8f);
    }

    Windows::UI::Color MainWindow::GetOutlineColorHSV(uint16_t age)
    {
        // should never see a black cell
        if (age > maxage)
        {
            return Windows::UI::Colors::Black();
        }

        const float h{ (age * 360.f) / maxage };
        return HSVtoColor(h, 0.8f, 0.9f);
    }

    void MainWindow::DrawInto(const CanvasDrawingSession& ds, uint16_t startY, uint16_t endY)
	{
        const float srcW{ _widthCellDest };
        const uint16_t srcStride{ gsl::narrow_cast<uint16_t>(std::sqrt(maxage) + 1) };
      
        float posx{ 0.0f };
        float posy{ startY * _widthCellDest };
        {
            for (uint16_t y = startY; y < endY; y++)
            {
                for (uint16_t x = 0; x < board.Width(); x++)
                {
                    if (const Cell& cell = board.GetCell(x, y); cell.IsAlive())
                    {
                        const int age = cell.Age() > maxage ? maxage : cell.Age();
                        const float srcX = gsl::narrow_cast<float>(age % srcStride);
                        const float srcY = gsl::narrow_cast<float>(age / srcStride);
                        const Windows::Foundation::Rect rectSrc{ srcW * srcX, srcW * srcY, srcW, srcW};
                        const Windows::Foundation::Rect rectDest{ posx, posy, _widthCellDest, _widthCellDest};

                        // this is not actually faster - unexpected
                        //sb.DrawFromSpriteSheet(_assets, rectDest, rectSrc);

                        // this is just as fast
                        ds.DrawImage(_assets, rectDest, rectSrc);

                        // good for debugging or perf comparisons
                        //ds.DrawRoundedRectangle(posx, posy, _widthCellDest, _widthCellDest, 2, 2, GetCellColorHSV(age));
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
            //_back = CanvasRenderTarget(device, bestbackbuffersize, bestbackbuffersize, theCanvas().Dpi());
            _widthCellDest = (bestbackbuffersize / _boardwidth);
        }
        InitializeAssets(device);
    }

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm
        sender;

        if (!board.IsDirty())
            return;

        CanvasDrawingSession ds = _back.CreateDrawingSession();
        ds.Clear(Colors::WhiteSmoke());
        ds.Antialiasing(CanvasAntialiasing::Aliased);
        ds.Blend(CanvasBlend::Copy);
        ds.Clear(Colors::WhiteSmoke());

        {
            std::scoped_lock lock{ lockboard };
            // render in 8 threads
            // see https://github.com/Microsoft/Win2D/issues/570
            auto drawinto1 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(0),                      gsl::narrow_cast<uint16_t>(board.Height() * 1 / 8));
            auto drawinto2 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 1 / 8), gsl::narrow_cast<uint16_t>(board.Height() * 2 / 8));
            auto drawinto3 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 2 / 8), gsl::narrow_cast<uint16_t>(board.Height() * 3 / 8));
            auto drawinto4 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 3 / 8), gsl::narrow_cast<uint16_t>(board.Height() * 4 / 8));
            auto drawinto5 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 4 / 8), gsl::narrow_cast<uint16_t>(board.Height() * 5 / 8));
            auto drawinto6 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 5 / 8), gsl::narrow_cast<uint16_t>(board.Height() * 6 / 8));
            auto drawinto7 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 6 / 8), gsl::narrow_cast<uint16_t>(board.Height() * 7 / 8));
            auto drawinto8 = std::async(std::launch::async, &MainWindow::DrawInto, this, std::ref(ds), gsl::narrow_cast<uint16_t>(board.Height() * 7 / 8), gsl::narrow_cast<uint16_t>(board.Height()));

            drawinto1.wait();
            drawinto2.wait();
            drawinto3.wait();
            drawinto4.wait();
            drawinto5.wait();
            drawinto6.wait();
            drawinto7.wait();
            drawinto8.wait();
        }

        ds.Flush();
        ds.Close();
    }

    int32_t MainWindow::SeedPercent() const noexcept
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

    int16_t MainWindow::BoardWidth() const noexcept
    {
        return _boardwidth;
    }

    void MainWindow::BoardWidth(int16_t value)
    {
        if (_boardwidth != value)
        {
            _boardwidth = value;
            timer.Stop();
            m_propertyChanged(*this, PropertyChangedEventArgs{ L"BoardWidth" });

            SetupRenderTargets();
            StartGameLoop();
        }
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
        canvasFmt.FontSize(gsl::narrow_cast<float>(PaneHeader().FontSize()));

        Brush backBrush{ splitView().PaneBackground() };
        Brush textBrush{ PaneHeader().Foreground() };

        SolidColorBrush scbBack = backBrush.try_as<SolidColorBrush>();
        SolidColorBrush scbText = textBrush.try_as<SolidColorBrush>();

        const Windows::UI::Color colorBack{scbBack.Color()};
        const Windows::UI::Color colorText{scbText.Color()};

        args.DrawingSession().Clear(colorBack);

        // create the strings to draw
        std::wstring strTitle = std::format(L"FPS\r\nGeneration\r\nAlive\r\nTotal Cells");
        std::wstring strContent = std::format(L"{}:{:.1f}\r\n{:8}\r\n{:8}\r\n{:8}", timer.FPS(), fps.FPS(), board.Generation(), board.GetLiveCount(), board.GetSize());
        
        // draw the text left aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Left);
        args.DrawingSession().DrawText(strTitle, 0, 0, 160, 100, colorText, canvasFmt);

        // draw the values right aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Right);
        args.DrawingSession().DrawText(strContent, 0, 0, 160, 100, colorText, canvasFmt);
        args.DrawingSession().Flush();

        sender.Invalidate();
    }

    void MainWindow::GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        sender;

        using namespace Microsoft::UI::Xaml::Controls;
        if (timer.IsRunning())
        {
            timer.Stop();
            GoButton().Label(L"Play");
            GoButton().Icon(SymbolIcon(Symbol::Play));

        }
        else
        {
            timer.Start();
            GoButton().Icon(SymbolIcon(Symbol::Pause));
            GoButton().Label(L"Pause");
        }
    }

    void MainWindow::RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        sender;

        using namespace Microsoft::UI::Xaml::Controls;

        timer.Stop();
        GoButton().Icon(SymbolIcon(Symbol::Pause));
        GoButton().Label(L"Pause");
        StartGameLoop();
    }

    void MainWindow::theCanvas_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        
        sender;
        //_canvasSize = min(e.NewSize().Width, bestcanvassize);
        _canvasSize = bestcanvassize;
        SetupRenderTargets();
    }

    hstring MainWindow::GetRandPercentText(double_t value)
    {
        std::wstring text = std::format(L"{0}% random", gsl::narrow_cast<int>(value));
        hstring htext{ text };
        return htext;
    }

    hstring MainWindow::GetBoardWidthText(double_t value)
    {
        std::wstring text = std::format(L"Width {0} x Height {0}", gsl::narrow_cast<int>(value));
        hstring htext{ text };
        return htext;
    }

    void MainWindow::speedClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        using namespace Microsoft::UI::Xaml::Controls;
        MenuFlyoutItem item = sender.try_as<MenuFlyoutItem>();
        
		dropdownSpeed().Content(winrt::box_value(item.Text()));

		timer.FPS(item.Tag().as<int>());
    }

    // Adapted from https://www.cs.rit.edu/~ncs/color/t_convert.html#RGB%20to%20XYZ%20&%20XYZ%20to%20RGB
    Windows::UI::Color MainWindow::HSVtoColor(float h, float s, float v)
    {
        if (h > 360.0f)
        {
            //return Windows::UI::Colors::Black();
            return ColorHelper::FromArgb(255, 196, 196, 196);
        }
        if (s == 0)
        {
            //return Windows::UI::Colors::Black();
            return ColorHelper::FromArgb(255, 228, 228, 228);
        }

        h /= 60;
        const int i{ gsl::narrow_cast<int>(std::floor(h)) };
        const float f{ h - i };
        const float p{ v * (1 - s) };
        const float q{ v * (1 - s * f) };
        const float t{ v * (1 - s * (1 - f)) };

        float dr{ 0 };
        float dg{ 0 };
        float db{ 0 };

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

        const uint8_t r{ gsl::narrow_cast<uint8_t>(dr * 255) };
        const uint8_t g{ gsl::narrow_cast<uint8_t>(dg * 255) };
        const uint8_t b{ gsl::narrow_cast<uint8_t>(db * 255)};

        return ColorHelper::FromArgb(255, r, g, b);
    }
}


