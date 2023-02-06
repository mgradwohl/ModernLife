// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include <winuser.h>
#include <algorithm>

#include "TimerHelper.h"
#include "fpscounter.h"

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

        PropertyChanged({ this, &MainWindow::OnPropertyChanged });

        SetMyTitleBar();
        OnBoardResized();
        SetBestCanvasandWindowSizes();

        _threadcount = SetThreadCount();

        timer.Tick({ this, &MainWindow::OnTick });
        StartGameLoop();
    }
    
    unsigned int MainWindow::SetThreadCount() noexcept
    {
        int count = gsl::narrow_cast<int>(std::thread::hardware_concurrency() / 2);
		count = std::clamp(count, 1, 8);
        return count;
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

    void MainWindow::Window_Closed([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) noexcept
    {
        //PropertyChangedRevoker();
    }

    void MainWindow::StartGameLoop()
    {
        // prep the play button
        timer.Stop();
        GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Play));
        GoButton().Label(L"Play");

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

    void MainWindow::RandomizeBoard()
    {
        // add a random population
        _board.RandomizeBoard(SeedPercent() / 100.0f, MaxAge());
    }
    
    void MainWindow::OnBoardResized()
    {
        // create the board, lock it in the case that OnTick is updating it
        // we lock it because changing board parameters will call StartGameLoop()
        {
            std::scoped_lock lock{ lockboard };
            _board = Board{ BoardWidth(), BoardWidth(), MaxAge()};
            RandomizeBoard();
        }

    }
    void MainWindow::SetupRenderTargets()
    {
        Microsoft::Graphics::Canvas::CanvasDevice device = Microsoft::Graphics::Canvas::CanvasDevice::GetSharedDevice();
        {
            // Calculate important vales for the spritesheet and backbuffer slices
            std::scoped_lock lock{ lockbackbuffer };

            _dipsPerCellDimension = _bestbackbuffersize / BoardWidth();
            _spritesPerRow = gsl::narrow_cast<uint16_t>(std::sqrt(MaxAge())) + 1;
            _spriteDipsPerRow = _dipsPerCellDimension * _spritesPerRow;
            _rowsPerSlice = gsl::narrow_cast<uint16_t>(_board.Height() / _threadcount);
            _sliceHeight = _rowsPerSlice * _dipsPerCellDimension;


            // create backbuffers that are sliced horizontally
            // they will be as evenly divided as possible
            // with the final slice potentially being larger
            _backbuffers.clear();
            int j = 0;
            for (j = 0; j < _threadcount - 1; j++)
            {
                _backbuffers.push_back(Microsoft::Graphics::Canvas::CanvasRenderTarget{ device, _bestbackbuffersize, _sliceHeight, _dpi });
            }
            const int remainingRows = BoardWidth() - (j * _rowsPerSlice);
            _backbuffers.push_back(Microsoft::Graphics::Canvas::CanvasRenderTarget{ device, _bestbackbuffersize, remainingRows * _dipsPerCellDimension, _dpi });
        }
        
        BuildSpriteSheet(device);

        if (!timer.IsRunning())
        {
            theCanvas().Invalidate();
        }
    }

    void MainWindow::theCanvas_CreateResources([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, [[maybe_unused]] Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs const& args)
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

        // determine the right size for the canvas
        {
            std::scoped_lock lock{ lockbackbuffer };
            _dpi = gsl::narrow_cast<float>(GetDpiForWindow(hWnd));

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

    void MainWindow::CanvasControl_Draw(Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl  const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        args.DrawingSession().Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Aliased);
        args.DrawingSession().Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);
        const Windows::Foundation::Rect destRect{ 0.0f, 0.0f, _bestcanvassize, _bestcanvassize };

        if (ShowLegend())
        {
            args.DrawingSession().DrawImage(_spritesheet, destRect);
        }
        else
        {
            // lock the full size backbuffer and copy each slice into it
            const float canvasSliceHeight = _sliceHeight / (_bestbackbuffersize / _bestcanvassize);
            Windows::Foundation::Rect source{ 0.0f, 0.0f, _bestbackbuffersize, _sliceHeight };
            Windows::Foundation::Rect dest{ 0.0f, 0.0f, _bestcanvassize,  canvasSliceHeight};
            {
                std::scoped_lock lock{ lockbackbuffer };

                args.DrawingSession().Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Aliased);
                args.DrawingSession().Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);

                int k = 0;
                for (k = 0; k < _threadcount - 1; k++)
                {
                    args.DrawingSession().DrawImage(gsl::at(_backbuffers, k), dest, source);
                    dest.Y += canvasSliceHeight;
                }
                dest.Height = _bestcanvassize - ( canvasSliceHeight * k);
                source.Height = _bestbackbuffersize - ( _sliceHeight* k) ;
                args.DrawingSession().DrawImage(gsl::at(_backbuffers, k), dest, source);

                args.DrawingSession().Flush();
                args.DrawingSession().Close();
            }
        }

        fps.AddFrame();
    }

    void MainWindow::DrawHorizontalRows(const Microsoft::Graphics::Canvas::CanvasDrawingSession& ds, uint16_t startRow, uint16_t endRow)
    {
        ds.Clear(Windows::UI::Colors::WhiteSmoke());
        ds.Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Antialiased);
        ds.Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);
        Microsoft::Graphics::Canvas::CanvasSpriteBatch spriteBatch = ds.CreateSpriteBatch(Microsoft::Graphics::Canvas::CanvasSpriteSortMode::Bitmap, Microsoft::Graphics::Canvas::CanvasImageInterpolation::NearestNeighbor, Microsoft::Graphics::Canvas::CanvasSpriteOptions::None);

        Windows::Foundation::Rect rectDest{ 0.0f, 0.0f, _dipsPerCellDimension, _dipsPerCellDimension };
        {
            for (uint16_t y = startRow; y < endRow; y++)
            {
                for (uint16_t x = 0; x < _board.Width(); x++)
                {
                    const Cell& cell = _board.GetCell(x, y);

                    rectDest.X = x * _dipsPerCellDimension;
                    rectDest.Y = (y - startRow) * _dipsPerCellDimension;
                    if (cell.IsAlive())
                    {
                        spriteBatch.DrawFromSpriteSheet(_spritesheet, rectDest, GetSpriteCell(cell.Age()));
                    }

                    if ((_ruleset == 4) && cell.IsBrianDying())
                    {
                        spriteBatch.DrawFromSpriteSheet(_spritesheet, rectDest, GetSpriteCell(cell.Age()));
                    }
                }
            }
        }
        spriteBatch.Close();
        ds.Flush();
        ds.Close();
    }

    void MainWindow::RenderOffscreen([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender)
    {
        //https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

        // create a drawing session for each backbuffer horizontal slice
        uint16_t startRow = 0;

        for (int j = 0; j < _threadcount; j++)
        {
            _dsList.push_back({ gsl::at(_backbuffers, j).CreateDrawingSession() });
        }

        // lock the board and draw the cells into the horizontal slices
        // note that the last slice may be bigger than the other slices
        {   
            std::scoped_lock lock{ lockboard };

            std::vector<std::thread> threads;
            int t = 0;
            for (t = 0; t < _threadcount-1; t++)
            {
                threads.push_back(std::thread{ &MainWindow::DrawHorizontalRows, this, gsl::at(_dsList , t), startRow, gsl::narrow_cast<uint16_t>(startRow + _rowsPerSlice)});
                startRow += _rowsPerSlice;
            }
            threads.push_back(std::thread{ &MainWindow::DrawHorizontalRows, this, gsl::at(_dsList , t), startRow, _board.Height()});

            // join the threads which waits for them to complete their work
            for (auto& th : threads)
			{
				th.join();
			}
        }
        _dsList.clear();
    }

    const Windows::Foundation::Rect MainWindow::GetSpriteCell(uint16_t index) const noexcept
    {
        const uint16_t i = index;
        i >= MaxAge() ? MaxAge() : i;
		const Windows::Foundation::Rect rect{ (i % _spritesPerRow) * _dipsPerCellDimension, (i / _spritesPerRow) * _dipsPerCellDimension, _dipsPerCellDimension, _dipsPerCellDimension };
       
        return rect;
    }

    void MainWindow::BuildSpriteSheet(const Microsoft::Graphics::Canvas::CanvasDevice& device)
    {
        // TODO only fill most of the cells with color. Reserve maybe the last 10% or so for gray
        // TODO gray is h=0, s=0, and v from 1 to 0
        // this will be used to iterate through the width and height of the rendertarget *without* adding a partial tile at the end of a row
        //_spritesPerRow = gsl::narrow_cast<uint16_t>(std::sqrt(MaxAge())) + 1;

        // create a square render target that will hold all the tiles (this will avoid a partial 'tile' at the end which we won't use)
        _spritesheet = Microsoft::Graphics::Canvas::CanvasRenderTarget(device, _spriteDipsPerRow, _spriteDipsPerRow, _dpi);

        Microsoft::Graphics::Canvas::CanvasDrawingSession ds = _spritesheet.CreateDrawingSession();
        //ds.Clear(Colors::Black());
        ds.Clear(Windows::UI::Colors::WhiteSmoke());
        ds.Antialiasing(Microsoft::Graphics::Canvas::CanvasAntialiasing::Antialiased);
        ds.Blend(Microsoft::Graphics::Canvas::CanvasBlend::Copy);
        

        // since we're using pixels, but the _dipsPerCellAxis is in dips
        // there is already "padding" in the sprite
        // so we'll take advantage of that
        // TODO maybe this is unneccessary

        // start filling tiles at age 0
        constexpr float offset = 1.0f;
        constexpr float round = 4.0f;
        const float inset = _dipsPerCellDimension / 4.0f;
        uint16_t index = 0;
        float posx{ 0.0f };
        float posy{ 0.0f };
        for (uint16_t y = 0; y < _spritesPerRow; y++)
        {
            for (uint16_t x = 0; x < _spritesPerRow; x++)
            {
                ds.FillRoundedRectangle(posx + offset, posy + offset, _dipsPerCellDimension - (2 * offset), _dipsPerCellDimension - (2 * offset), round, round, GetOutlineColorHSV(index));
                ds.FillRoundedRectangle(posx + inset, posy + inset, _dipsPerCellDimension - (2 * inset), _dipsPerCellDimension - (2 * inset), round, round, GetCellColorHSV(index));

                posx += _dipsPerCellDimension;
                index++;
            }
            posy += _dipsPerCellDimension;
            posx = 0.0f;
        }
      
        ds.Flush();
        ds.Close();
    }

    void MainWindow::theCanvasStatsContent_Draw([[maybe_unused]] Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
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
        std::wstring strTitle = std::format(L"FPS\r\nGeneration\r\nAlive\r\nTotal Cells\r\n\r\nDPI\r\nCanvas Size\r\nBackbuffer Size\r\nCell Size\r\nThreads");
        std::wstring strContent = std::format(L"{}:{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:8L}\r\n\r\n{:.1f}\r\n{:8L}\r\n{:8L}\r\n{:.2f}\r\n{:8L}", timer.FPS(), fps.FPS(), _board.Generation(), _board.GetLiveCount(), _board.GetSize(), _dpi, _bestcanvassize, _bestbackbuffersize, _dipsPerCellDimension, _threadcount);

        // draw the text left aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Left);
        args.DrawingSession().DrawText(strTitle, 0, 0, 160, 100, colorText, canvasFmt);

        // draw the values right aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Right);
        args.DrawingSession().DrawText(strContent, 0, 0, 160, 100, colorText, canvasFmt);
        args.DrawingSession().Flush();

        sender.Invalidate();
    }

    void MainWindow::OnMaxAgeChanged()
    {
        _board.SetMaxAge(MaxAge());
        SetupRenderTargets();
    }
    
    void MainWindow::OnPropertyChanged([[maybe_unused]] IInspectable const& sender, PropertyChangedEventArgs const& args)
    {
        if (args.PropertyName() == L"MaxAge")
        {
            OnMaxAgeChanged();
        }

        if (args.PropertyName() == L"BoardWidth")
        {
            timer.Stop();
            OnBoardResized();
            SetupRenderTargets();
            StartGameLoop();
        }

        if (args.PropertyName() == L"ShowLegend")
        {
            theCanvas().Invalidate();
        }

        if (args.PropertyName() == L"SeedPercent")
        {
            RandomizeBoard();
            StartGameLoop();
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
    
    uint16_t MainWindow::SeedPercent() const noexcept
    {
        return _randompercent;
    }

    void MainWindow::SeedPercent(uint16_t value)
    {
        if (_randompercent != value)
        {
            _randompercent = value;
            _propertyChanged(*this, PropertyChangedEventArgs{ L"SeedPercent" });
        }
    }

    uint16_t MainWindow::BoardWidth() const noexcept
    {
        return _boardwidth;
    }

    void MainWindow::BoardWidth(uint16_t value)
    {
        if (_boardwidth != value)
        {
            _boardwidth = value;
            _propertyChanged(*this, PropertyChangedEventArgs{ L"BoardWidth" });
        }
    }

    void MainWindow::GoButton_Click([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (timer.IsRunning())
        {
            timer.Stop();
            GoButton().Label(L"Play");
            GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Play));

        }
        else
        {
            timer.Start();
            GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Pause));
            GoButton().Label(L"Pause");
        }
    }

    void MainWindow::RestartButton_Click([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        timer.Stop();
        GoButton().Icon(Microsoft::UI::Xaml::Controls::SymbolIcon(Microsoft::UI::Xaml::Controls::Symbol::Pause));
        GoButton().Label(L"Pause");
        StartGameLoop();
    }

    void MainWindow::theCanvas_SizeChanged([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
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

    void MainWindow::speedClick(IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        Microsoft::UI::Xaml::Controls::MenuFlyoutItem item = sender.try_as<Microsoft::UI::Xaml::Controls::MenuFlyoutItem>();
		dropdownSpeed().Content(winrt::box_value(item.Text()));

		timer.FPS(item.Tag().as<int>());
    }

    void MainWindow::ruleClick(IInspectable const& sender, [[maybe_unused]] winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        Microsoft::UI::Xaml::Controls::MenuFlyoutItem item = sender.try_as<Microsoft::UI::Xaml::Controls::MenuFlyoutItem>();
        dropdownRules().Content(winrt::box_value(item.Text()));

        _ruleset = item.Tag().as<int>();
    }

    Windows::UI::Color MainWindow::GetOutlineColorHSV(uint16_t age)
    {
        if (age >= MaxAge())
        {
            return Windows::UI::Colors::DarkGray();
        }

        const float h{ (age * 360.f) / MaxAge()};
        return HSVtoColor(h, 0.6f, 0.7f);
    }

    Windows::UI::Color MainWindow::GetCellColorHSV(uint16_t age)
    {
        if (age >= MaxAge())
        {
            return Windows::UI::Colors::Gray();
        }

        const float h{ (age * 360.f) / MaxAge()};
        return HSVtoColor(h, 0.7f, 0.9f);
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

        float dr{ 0.0f };
        float dg{ 0.0f };
        float db{ 0.0f };

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

        return Windows::UI::ColorHelper::FromArgb(255, r, g, b);
    }
}