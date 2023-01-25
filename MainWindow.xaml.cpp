// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winuser.h>
#include "TimerHelper.h"
#include "fpscounter.h"

using namespace Microsoft::Graphics::Canvas;

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
        HWND hWnd{ nullptr };
        winrt::check_hresult(windowNative->get_WindowHandle(&hWnd));
        const Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(hWnd);

        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            Windows::Graphics::PointInt32 pos{ appWnd.Position() };
            pos.Y = 32;
            appWnd.Move(pos);
            
            const float dpi = gsl::narrow_cast<float>(GetDpiForWindow(hWnd));
            const int wndWidth = gsl::narrow_cast<int>((bestcanvassize + 240) * dpi / 96.0f);
            const int wndHeight = gsl::narrow_cast<int>((bestcanvassize + 40) * dpi / 96.0f);
			appWnd.ResizeClient(Windows::Graphics::SizeInt32{ wndWidth, wndHeight });
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

    void MainWindow::BuildSpriteSheet(const CanvasDevice& device)
    {
        // this will be used to iterate through the width and height of the rendertarget *without* adding a partial tile at the end of a row
        const uint16_t assetStride = gsl::narrow_cast<uint16_t>(std::sqrt(maxage)) + 1;

        // create a square render target that will hold all the tiles (this will avoid a partial 'tile' at the end which we won't use)
        const float rtsize = _widthCellDest * assetStride;
        _spritesheet = CanvasRenderTarget(device, rtsize, rtsize, theCanvas().Dpi());

        CanvasDrawingSession ds = _spritesheet.CreateDrawingSession();
        ds.Clear(Colors::WhiteSmoke());
        ds.Antialiasing(CanvasAntialiasing::Antialiased);

        float posx{ 0.0f };
        float posy{ 0.0f };

        float round{ 2.0f };
        float offset{ 1.0f };
        if (_widthCellDest > 20)
        {
            round = 6.0f;
            offset = 2.0f;
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
            _board.FastUpdateBoardWithNextState(_ruleset);
            _board.ApplyNextStateToBoard();
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
            _board = Board{ gsl::narrow_cast<uint16_t>(_boardwidth), gsl::narrow_cast<uint16_t>(_boardwidth) };
        }

        // add a random population
        auto randomizer = std::async(&Board::RandomizeBoard, &_board, _randompercent / 100.0f);
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
            // we lock the back buffer so that RenderOffscreen is not changing it while we draw it
            std::scoped_lock lock{ lockbackbuffer };
            args.DrawingSession().Antialiasing(CanvasAntialiasing::Aliased);
            args.DrawingSession().Blend(CanvasBlend::Copy);

			//Windows::Foundation::Rect sourceRect{ 0.0f, 0.0f, _back.Size().Width, _back.Size().Height };
            const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _canvasSize, _canvasSize};

            // draw the backbuffer to the control - they are likely different sizes
            // comment out the following to see the sprite sheet
            args.DrawingSession().DrawImage(_backbuffer, destRect);
            
            // uncomment the following line to see the sprite sheet
            //args.DrawingSession().DrawImage(_assets, 0, 0);
            args.DrawingSession().Flush();
        }
        fps.AddFrame();
    }

    void MainWindow::DrawHorizontalRows(const CanvasDrawingSession& ds, uint16_t startY, uint16_t endY)
	{
        const float srcW{ _widthCellDest };
        const uint16_t srcStride{ gsl::narrow_cast<uint16_t>(std::sqrt(maxage) + 1) };

        CanvasSpriteBatch spriteBatch = ds.CreateSpriteBatch(CanvasSpriteSortMode::None, CanvasImageInterpolation::NearestNeighbor, CanvasSpriteOptions::ClampToSourceRect);

        float posx{ 0.0f };
        float posy{ startY * _widthCellDest };
        {
            for (uint16_t y = startY; y < endY; y++)
            {
                for (uint16_t x = 0; x < _board.Width(); x++)
                {
                    if (const Cell& cell = _board.GetCell(x, y); cell.IsAlive())
                    {
                        const int age = cell.Age() > maxage ? maxage : cell.Age();
                        const float srcX = gsl::narrow_cast<float>(age % srcStride);
                        const float srcY = gsl::narrow_cast<float>(age / srcStride);
                        const Windows::Foundation::Rect rectSrc{ srcW * srcX, srcW * srcY, srcW, srcW};
                        const Windows::Foundation::Rect rectDest{ posx, posy, _widthCellDest, _widthCellDest};

                        spriteBatch.DrawFromSpriteSheet(_spritesheet, rectDest, rectSrc);
                    }
                    posx += _widthCellDest;
                }
                posy += _widthCellDest;
                posx = 0.0f;
            }
        }
        spriteBatch.Close();
	}

    void MainWindow::SetupRenderTargets()
    {
        CanvasDevice device = CanvasDevice::GetSharedDevice();

        {
			// lock the backbuffer because the it's being recreated and we don't want RenderOffscreen or Draw to use it while it's being recreated
            std::scoped_lock lock{ lockbackbuffer };
            _backbuffer = CanvasRenderTarget(device, bestbackbuffersize, bestbackbuffersize, theCanvas().Dpi());
            _widthCellDest = (bestbackbuffersize / _boardwidth);
        }
        BuildSpriteSheet(device);
    }

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm
        sender;

        CanvasDrawingSession ds = _backbuffer.CreateDrawingSession();
        ds.Clear(Colors::WhiteSmoke());
        ds.Antialiasing(CanvasAntialiasing::Aliased);
        ds.Blend(CanvasBlend::Copy);

        {
            std::scoped_lock lock{ lockboard };
            // render in threads - doesn't actually seem to render in 8 threads, see https://github.com/Microsoft/Win2D/issues/570
            auto drawinto1 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(0),                      gsl::narrow_cast<uint16_t>(_board.Height() * 1 / 8));
            auto drawinto2 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 1 / 8), gsl::narrow_cast<uint16_t>(_board.Height() * 2 / 8));
            auto drawinto3 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 2 / 8), gsl::narrow_cast<uint16_t>(_board.Height() * 3 / 8));
            auto drawinto4 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 3 / 8), gsl::narrow_cast<uint16_t>(_board.Height() * 4 / 8));
            auto drawinto5 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 4 / 8), gsl::narrow_cast<uint16_t>(_board.Height() * 5 / 8));
            auto drawinto6 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 5 / 8), gsl::narrow_cast<uint16_t>(_board.Height() * 6 / 8));
            auto drawinto7 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 6 / 8), gsl::narrow_cast<uint16_t>(_board.Height() * 7 / 8));
            auto drawinto8 = std::async(std::launch::async, &MainWindow::DrawHorizontalRows, this, std::ref(ds), gsl::narrow_cast<uint16_t>(_board.Height() * 7 / 8), gsl::narrow_cast<uint16_t>(_board.Height()));

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

    void MainWindow::theCanvasStatsContent_Draw(CanvasControl const& sender, CanvasDrawEventArgs const& args)
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
        std::wstring strContent = std::format(L"{}:{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:8L}", timer.FPS(), fps.FPS(), _board.Generation(), _board.GetLiveCount(), _board.GetSize());
        
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
        e;
        sender;
        // this locks the canvas size, but can we let the user resize and if it's too big they can scroll and zoom?
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

    void MainWindow::ruleClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        using namespace Microsoft::UI::Xaml::Controls;
        MenuFlyoutItem item = sender.try_as<MenuFlyoutItem>();

        dropdownRules().Content(winrt::box_value(item.Text()));

        _ruleset = item.Tag().as<int>();
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

    // Adapted from https://www.cs.rit.edu/~ncs/color/t_convert.html#RGB%20to%20XYZ%20&%20XYZ%20to%20RGB
    Windows::UI::Color MainWindow::HSVtoColor(float h, float s, float v)
    {
        if (h > 360.0f)
        {
            //return Windows::UI::Colors::Black();
			return Windows::UI::Colors::DarkSlateGray();
        }
        if (s == 0)
        {
            //return Windows::UI::Colors::Black();
            return Windows::UI::Colors::DarkSlateGray();
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

