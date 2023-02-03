// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winuser.h>
#include "TimerHelper.h"
#include "fpscounter.h"

using namespace Microsoft::Graphics::Canvas;

namespace winrt::ModernLife::implementation
{
    MainWindow::MainWindow() noexcept
    {
        //https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        //InitializeComponent(); 
    }

    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();

        SetMyTitleBar();
        SetBestCanvasandWindowSizes();

        // doesn't work see TimerHelper.h
        //timer = TimerHelper({ this, &MainWindow::OnTick }, 60, true);

        PropertyChanged({ this, &MainWindow::OnPropertyChanged });
        //_propertyToken = m_propertyChanged.add({ this, &MainWindow::OnPropertyChanged });
        timer.Tick({ this, &MainWindow::OnTick });
        StartGameLoop();
    }
    
    void MainWindow::SetMyTitleBar()
    {
        // Set window title
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());

        // get window handle, window ID
        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ nullptr };
        winrt::check_hresult(windowNative->get_WindowHandle(&hWnd));
        const Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(hWnd);

        // set the title
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

    void MainWindow::OnWindowActivate([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] WindowActivatedEventArgs const& args)
    {
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

    void MainWindow::Window_Closed([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept
    {
        //PropertyChangedRevoker();
    }

    void MainWindow::StartGameLoop()
    {
        // prep the play button
        timer.Stop();
        using namespace Microsoft::UI::Xaml::Controls;
        GoButton().Icon(SymbolIcon(Symbol::Play));
        GoButton().Label(L"Play");

        // create the board, lock it in the case that OnTick is updating it
        // we lock it because changing board parameters will call StartGameLoop()
        {
            std::scoped_lock lock{ lockboard };
            _board = Board{ gsl::narrow_cast<uint16_t>(BoardWidth()), gsl::narrow_cast<uint16_t>(BoardWidth()) };

            // add a random population
            //auto randomizer = std::async(&Board::RandomizeBoard, &_board, SeedPercent() / 100.0f, MaxAge());
            //randomizer.wait();

            _board.RandomizeBoard(SeedPercent() / 100.0f, MaxAge());
        }

        // start the FPSCounter
        fps = FPScounter();

        // draw the initial population
        theCanvas().Invalidate();
    }

    void MainWindow::OnTick(winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const&, IInspectable const&)
    {
        {
            std::scoped_lock lock{ lockboard };
            _board.FastUpdateBoardWithNextState(_ruleset);
            _board.ApplyNextStateToBoard();
        }
        theCanvas().Invalidate();
    }

    void MainWindow::SetupRenderTargets()
    {
        CanvasDevice device = CanvasDevice::GetSharedDevice();
        _widthCellStride = _bestbackbuffersize / BoardWidth();
        _widthCellDest = std::floor(_widthCellStride);

        {
            // lock the backbuffer because the it's being recreated and we don't want RenderOffscreen or Draw to use it while it's being recreated
            std::scoped_lock lock{ lockbackbuffer };
            _backbuffer = CanvasRenderTarget(device, _bestbackbuffersize, _bestbackbuffersize, _dpi);

            const float cellsPerSlice =  std::floor(_bestbackbuffersize / _threadcount / _widthCellStride);
            float rowsHandled = 0.0f;
            _backbuffers.clear();
            for (int j = 0; j < _threadcount -1 ; j++)
            {
                _backbuffers.push_back(CanvasRenderTarget{ device, _bestbackbuffersize, cellsPerSlice * _widthCellStride, _dpi });
                rowsHandled += cellsPerSlice;
            }
            _backbuffers.push_back(CanvasRenderTarget{ device, _bestbackbuffersize, (BoardWidth() - rowsHandled) * _widthCellStride, _dpi});

        }

        BuildSpriteSheet(device);

        if (!timer.IsRunning())
        {
            theCanvas().Invalidate();
        }
    }

    void MainWindow::theCanvas_CreateResources([[maybe_unused]] CanvasControl const& sender, [[maybe_unused]] Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args)
    {
        // todo might want to do the code in the if-block in all cases (for all args.Reason()s
        //if (args.Reason() == Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason::DpiChanged)
        {
            // get window handle, window ID
            auto windowNative{ this->try_as<::IWindowNative>() };
            HWND hWnd{ nullptr };
            winrt::check_hresult(windowNative->get_WindowHandle(&hWnd));

            SetBestCanvasandWindowSizes();
            SetupRenderTargets();
            theCanvas().Invalidate();
        }
    }

    void MainWindow::SetBestCanvasandWindowSizes()
    {
        // get window handle, window ID
        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ nullptr };
        winrt::check_hresult(windowNative->get_WindowHandle(&hWnd));
        const Microsoft::UI::WindowId idWnd = Microsoft::UI::GetWindowIdFromWindow(hWnd);

        // get the window size
        Microsoft::UI::Windowing::DisplayArea displayAreaFallback(nullptr);
        Microsoft::UI::Windowing::DisplayArea displayArea = Microsoft::UI::Windowing::DisplayArea::GetFromWindowId(idWnd, Microsoft::UI::Windowing::DisplayAreaFallback::Nearest);
        const Windows::Graphics::RectInt32 rez = displayArea.OuterBounds();

        _dpi = gsl::narrow_cast<float>(GetDpiForWindow(hWnd));

        // determine the right size for the canvas
        float best = 400.0f;
        while (true)
        {
            if ((best * _dpi / 96.0f) >= rez.Height) break;
            best += 100.0f;
        }
        best -= 200.0f;
        _bestcanvassize = best;

        // make the backbuffer bigger than the front buffer, and a multiple of it
        _bestbackbuffersize = _bestcanvassize;
        while (_bestbackbuffersize < _idealbackbuffersize)
        {
            _bestbackbuffersize += _bestcanvassize;
        }

        // setup offsets for sensible default window size
        constexpr int border = 20;
        constexpr int stackpanelwidth = 200;
        const int wndWidth = gsl::narrow_cast<int>((_bestcanvassize + stackpanelwidth + border) * _dpi / 96.0f);
        const int wndHeight = gsl::narrow_cast<int>((_bestcanvassize + border) * _dpi / 96.0f);

        // set the title, set the window size, and move the window
        if (auto appWnd = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(idWnd); appWnd)
        {
            appWnd.ResizeClient(Windows::Graphics::SizeInt32{ wndWidth, wndHeight });
        }
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        args.DrawingSession().Antialiasing(CanvasAntialiasing::Aliased);
        args.DrawingSession().Blend(CanvasBlend::Copy);
        const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _canvasSize, _canvasSize };

        if (ShowLegend())
        {
            args.DrawingSession().DrawImage(_spritesheet, destRect);
        }
        else
        {
            std::scoped_lock lock{ lockbackbuffer };
            args.DrawingSession().DrawImage(_backbuffer, destRect);
        }
        args.DrawingSession().Flush();

        fps.AddFrame();
    }

    void MainWindow::DrawHorizontalRows(const CanvasDrawingSession& ds, int startRow, int endRow)
    {
        ds.Clear(Colors::WhiteSmoke());
        //ds.Clear(ColorHelper::FromArgb(255, 0, 0, gsl::narrow_cast<uint8_t>(endRow)));
        ds.Antialiasing(CanvasAntialiasing::Aliased);
        ds.Blend(CanvasBlend::Copy);
        CanvasSpriteBatch spriteBatch = ds.CreateSpriteBatch(CanvasSpriteSortMode::None, CanvasImageInterpolation::NearestNeighbor, CanvasSpriteOptions::ClampToSourceRect);

        Windows::Foundation::Rect rectDest{ 0.0f, 0.0f, _widthCellStride, _widthCellStride };
        {
            for (uint16_t y = startRow; y < endRow; y++)
            {
                for (uint16_t x = 0; x < _board.Width(); x++)
                {
                    const Cell& cell = _board.GetCell(x, y);
                    const int age = cell.Age() > MaxAge() ? MaxAge() : cell.Age();

                    rectDest.X = x * _widthCellStride;
                    rectDest.Y = (y - startRow) * _widthCellStride;
                    if (cell.IsAlive())
                    {
                            spriteBatch.DrawFromSpriteSheet(_spritesheet, rectDest, GetSpriteCell(age));
                    }

                    if ((_ruleset == 4) && cell.IsBrianDying())
                    {
                        spriteBatch.DrawFromSpriteSheet(_spritesheet, rectDest, GetSpriteCell(age));
                    }
                }
            }
        }
        spriteBatch.Close();
        ds.Flush();
        ds.Close();
    }

    void MainWindow::RenderOffscreen([[maybe_unused]] CanvasControl const& sender)
    {
        //https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm
        const int rowsPerSlice = std::floor(_bestbackbuffersize / _threadcount / _widthCellStride);
        int startRow = 0;

        std::vector<CanvasDrawingSession> dsList;
        for (int j = 0; j < _threadcount; j++)
		{
            dsList.push_back({ _backbuffers[j].CreateDrawingSession() });
		}

        {
            std::scoped_lock lock{ lockboard };

            std::vector<std::thread> threads;
            for (int t = 0; t < _threadcount-1; t++)
            {
                threads.push_back(std::thread{ &MainWindow::DrawHorizontalRows, this, dsList[t], startRow, startRow + rowsPerSlice});
                startRow += rowsPerSlice;
            }
            threads.push_back(std::thread{ &MainWindow::DrawHorizontalRows, this, dsList[_threadcount-1], startRow, _board.Height()});

			for (auto& th : threads)
			{
				th.join();
			}
        }

        const float sliceHeight{ rowsPerSlice* _widthCellStride };
        Windows::Foundation::Rect source{ 0.0f, 0.0f, _bestbackbuffersize, sliceHeight};
        Windows::Foundation::Rect dest{ 0.0f, 0.0f, _bestbackbuffersize, sliceHeight};

        {
            std::scoped_lock lock{ lockbackbuffer };

            CanvasDrawingSession ds = _backbuffer.CreateDrawingSession();
            ds.Antialiasing(CanvasAntialiasing::Aliased);
            ds.Blend(CanvasBlend::Copy);

            for (int k = 0; k < _threadcount-1; k++)
            {
#ifdef _DEBUG
                //ds.DrawRectangle(dest, Colors::Black(), 8.0f);
#endif
                ds.DrawImage(_backbuffers[k], dest, source);
                dest.Y += sliceHeight;
            }
            dest.Height = (_board.Height() - startRow) * _widthCellStride;
            source.Height = (_board.Height() - startRow) * _widthCellStride;
            ds.DrawImage(_backbuffers[_threadcount-1], dest, source);

            ds.Flush();
            ds.Close();
        }
    }

    Windows::Foundation::Rect& MainWindow::GetSpriteCell(int index)
    {
        const uint16_t spritesPerAxis = gsl::narrow_cast<uint16_t>(std::sqrt(MaxAge())) + 1;

		Windows::Foundation::Rect rect{ (index / spritesPerAxis) * _widthCellStride, (index % spritesPerAxis)* _widthCellStride, _widthCellStride, _widthCellStride };
       
        return rect;
    }

    void MainWindow::BuildSpriteSheet(const CanvasDevice& device)
    {
        // TODO only fill most of the cells with color. Reserve maybe the last 10% or so for gray
        // TODO gray is h=0, s=0, and v from 1 to 0
        // this will be used to iterate through the width and height of the rendertarget *without* adding a partial tile at the end of a row
        const uint16_t cellsPerAxis = gsl::narrow_cast<uint16_t>(std::sqrt(MaxAge())) + 1;

        // create a square render target that will hold all the tiles (this will avoid a partial 'tile' at the end which we won't use)
        const float rtsize = _widthCellStride * cellsPerAxis;
        _spritesheet = CanvasRenderTarget(device, rtsize, rtsize, _dpi);

        CanvasDrawingSession ds = _spritesheet.CreateDrawingSession();
#ifdef _DEBUG
        ds.Clear(Colors::Black());
#else
        ds.Clear(Colors::WhiteSmoke());
#endif
        ds.Antialiasing(CanvasAntialiasing::Antialiased);
        ds.Blend(CanvasBlend::Copy);
        
        float posx{ 0.0f };
        float posy{ 0.0f };

        const float round{ _widthCellDest * 0.2f };
        float offset = gsl::narrow_cast<float>(1.0f - (_widthCellDest - _widthCellStride));
        if (offset < 0.5f)
        {
            offset = 0.5f;
        };

        // start filling tiles at age 0
        uint16_t index = 0;
        for (uint16_t y = 0; y < cellsPerAxis; y++)
        {
            for (uint16_t x = 0; x < cellsPerAxis; x++)
            {
                ds.FillRoundedRectangle(posx + offset, posy + offset, _widthCellStride - (2 * offset), _widthCellStride - (2 * offset), round, round, GetCellColorHSV(index));
                ds.DrawRoundedRectangle(posx + offset, posy + offset, _widthCellStride - (2 * offset), _widthCellStride - (2 * offset), round, round, GetOutlineColorHSV(index), 1.0f);
                posx += _widthCellStride;
                index++;
            }
            posy += _widthCellStride;
            posx = 0.0f;
        }
        ds.Flush();
        ds.Close();
    }

    void MainWindow::theCanvasStatsContent_Draw([[maybe_unused]] CanvasControl const& sender, CanvasDrawEventArgs const& args)
    {
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

        const Windows::UI::Color colorBack{ scbBack.Color() };
        const Windows::UI::Color colorText{ scbText.Color() };

        args.DrawingSession().Clear(colorBack);

        // create the strings to draw
        std::wstring strTitle = std::format(L"FPS\r\nGeneration\r\nAlive\r\nTotal Cells\r\n\r\nDPI\r\nCanvas Size\r\nBackbuffer Size\r\nCell Size");
        std::wstring strContent = std::format(L"{}:{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:8L}\r\n\r\n{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:.2f}", timer.FPS(), fps.FPS(), _board.Generation(), _board.GetLiveCount(), _board.GetSize(), _dpi, _canvasSize, _bestbackbuffersize, _widthCellStride);

        // draw the text left aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Left);
        args.DrawingSession().DrawText(strTitle, 0, 0, 160, 100, colorText, canvasFmt);

        // draw the values right aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Right);
        args.DrawingSession().DrawText(strContent, 0, 0, 160, 100, colorText, canvasFmt);
        args.DrawingSession().Flush();

        sender.Invalidate();
    }

    void MainWindow::OnPropertyChanged([[maybe_unused]] IInspectable const& sender, PropertyChangedEventArgs const& args)
    {
        if (args.PropertyName() == L"MaxAge")
        {
            SetupRenderTargets();
        }

        if (args.PropertyName() == L"BoardWidth")
        {
            SetupRenderTargets();
            StartGameLoop();
        }
        if (args.PropertyName() == L"ShowLegend")
        {
            theCanvas().Invalidate();
        }
    }

    int32_t MainWindow::MaxAge() const noexcept
    {
        return _maxage;
    }

    void MainWindow::MaxAge(int32_t value)
    {
        if (_maxage != value)
        {
            _maxage = value;

            _propertyChanged(*this, PropertyChangedEventArgs{ L"MaxAge" });
        }
    }

    bool MainWindow::ShowLegend() const noexcept
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
    
    int32_t MainWindow::SeedPercent() const noexcept
    {
        return _randompercent;
    }

    void MainWindow::SeedPercent(int32_t value)
    {
        if (_randompercent != value)
        {
            _randompercent = value;

            _propertyChanged(*this, PropertyChangedEventArgs{ L"SeedPercent" });
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
            _propertyChanged(*this, PropertyChangedEventArgs{ L"BoardWidth" });
        }
    }

    void MainWindow::GoButton_Click([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
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

    void MainWindow::RestartButton_Click([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        using namespace Microsoft::UI::Xaml::Controls;

        timer.Stop();
        GoButton().Icon(SymbolIcon(Symbol::Pause));
        GoButton().Label(L"Pause");
        StartGameLoop();
    }

    void MainWindow::theCanvas_SizeChanged([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        // this locks the canvas size, but can we let the user resize and if it's too big they can scroll and zoom?
        _canvasSize = _bestcanvassize;
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

    void MainWindow::speedClick(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        using namespace Microsoft::UI::Xaml::Controls;
        MenuFlyoutItem item = sender.try_as<MenuFlyoutItem>();
        
		dropdownSpeed().Content(winrt::box_value(item.Text()));

		timer.FPS(item.Tag().as<int>());
    }

    void MainWindow::ruleClick(IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        using namespace Microsoft::UI::Xaml::Controls;
        MenuFlyoutItem item = sender.try_as<MenuFlyoutItem>();

        dropdownRules().Content(winrt::box_value(item.Text()));

        _ruleset = item.Tag().as<int>();
    }

    Windows::UI::Color MainWindow::GetCellColorHSV(uint16_t age)
    {
        if (age >= MaxAge())
        {
            return Windows::UI::Colors::DarkGray();
        }

        const float h{ (age * 360.f) / MaxAge()};
        return HSVtoColor(h, 0.6f, 0.8f);
    }

    Windows::UI::Color MainWindow::GetOutlineColorHSV(uint16_t age)
    {
        if (age >= MaxAge())
        {
            return Windows::UI::Colors::DarkSlateGray();
        }

        const float h{ (age * 360.f) / MaxAge()};
        return HSVtoColor(h, 0.8f, 0.9f);
    }

    // Adapted from https://www.cs.rit.edu/~ncs/color/t_convert.html#RGB%20to%20XYZ%20&%20XYZ%20to%20RGB
    Windows::UI::Color MainWindow::HSVtoColor(float h, float s, float v)
    {
        if (h > 360.0f)
        {
            return Windows::UI::Colors::Black();
        }
        if (s == 0)
        {
            return Windows::UI::Colors::Black();
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