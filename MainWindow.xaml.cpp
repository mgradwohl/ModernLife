// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include<future>
#include<format>

#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ModernLife::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        StartGameLoop();
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
        // create the board
        {
            std::scoped_lock lock{ lockboard };
            board = Board{ static_cast<uint16_t>(_boardwidth), static_cast<uint16_t>(_boardwidth) };
        }

        auto randomizer = std::async(&Board::RandomizeBoard, &board, _randompercent / 100.0f);
        randomizer.wait();

        // create and start a timer
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

        using namespace  std::literals::chrono_literals;
        _timer.Interval(std::chrono::milliseconds{ 33 });
        _timer.IsRepeating(true);

        if (! _tokeninit)
        {
            winrt::event_token _registrationtoken = _timer.Tick({ this, &MainWindow::OnTick });
            _tokeninit = true;
        }

        using namespace Microsoft::UI::Xaml::Controls;
        GoButton().Icon(SymbolIcon(Symbol::Play));
        GoButton().Label(L"Play");

        theCanvas().Invalidate();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        {
            std::scoped_lock lock{ lockbackbuffer };
            args.DrawingSession().DrawImage(_back, 0, 0);
        }
    }

    Windows::UI::Color MainWindow::GetCellColor2(const Cell& cell)
    {
        if (!_colorinit)
        {
            for (uint16_t index = 0; index <= maxage + 1; index++)
            {
                uint8_t a = 255;
                uint8_t r = static_cast<uint8_t>((index * 255)/maxage);
                uint8_t g = 128;
                uint8_t b = 128;
                vecColors.emplace_back(ColorHelper::FromArgb(a, r, g, b));

            }

            // setup vector of colors
            _colorinit = true;
        }
        uint16_t age = cell.Age() > maxage ? maxage : cell.Age();

        return vecColors[age];
    }

    Windows::UI::Color MainWindow::GetCellColor3(const Cell& cell)
    {
        if (!_colorinit)
        {
            float h = 0.0f;

            vecColors.resize(maxage + 1);
            for (int index = 100; index < maxage - 100 + 1; index++)
            {
                h = ((float)index) / maxage * 360.0f;
                vecColors[index] = HSVtoRGB2(h, 60.0, 60.0);
            }
            _colorinit = true;
        }

        if (cell.Age() < 100)
        {
            return Windows::UI::Colors::Green();
        }
        
        if (cell.Age() > maxage - 100)
        {
            return Windows::UI::Colors::Black();
        }

        int age = cell.Age() > maxage ? maxage : cell.Age();

        return vecColors[age];
    }

    Windows::UI::Color MainWindow::GetCellColor(const Cell& cell) const
    {
        uint8_t colorscale = 0;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;

        colorscale = static_cast<uint8_t>((cell.Age() * 255) / maxage);
        colorscale = 254 - colorscale;

        if (cell.Age() <= (maxage * 1/4))
        {
            green = colorscale;
        }
        else if (cell.Age() > (maxage * 1/4) && cell.Age() <= (maxage * 1/2))
        {
            blue = colorscale;
        }
        else if (cell.Age() > (maxage * 1/2) && cell.Age() <= (maxage * 3/4))
        {
            red = colorscale;
        }
        else if (cell.Age() > (maxage * 3/4) && cell.Age() <= maxage)
        {
            red = colorscale;
            green = colorscale;
            blue = colorscale;
        }
        Windows::UI::Color cellcolor = ColorHelper::FromArgb(255, red, green, blue);
        return cellcolor;
    }

    void MainWindow::DrawInto(CanvasDrawingSession& ds, uint16_t startY, uint16_t endY, float width)
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

        float w = (width / _boardwidth);
		float posx = 0.0f;
		float posy = startY * w;
        {
            std::scoped_lock lock{ lockboard };

            for (uint16_t y = startY; y < endY; y++)
            {
                for (uint16_t x = 0; x < board.Width(); x++)
                {
                    if (const Cell& cell = board.GetCell(x, y); cell.IsAlive())
                    {
                        ds.DrawRoundedRectangle(posx, posy, w, w, 2, 2, GetCellColor3(cell));
                    }
                    posx += w;
                }
                posy += w;
                posx = 0.0f;
            }
        }
	}

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

        if (!board.IsDirty())
            return;

        constexpr int bestsize = 1250;// cellcount * 5;
        winrt::Windows::Foundation::Size huge = sender.Size();
        float width = min(huge.Width, bestsize);
        float height = width;// (width / cellcount)* board.Height();

        // if the back buffer doesn't exist or is the wrong size, create it
        if (nullptr == _back || _back.Size() != sender.Size())
        {
            CanvasDevice device = CanvasDevice::GetSharedDevice();
            {
                std::scoped_lock lock{ lockbackbuffer };
                _back = CanvasRenderTarget(device, width, height, sender.Dpi());
            }
        }

        CanvasDrawingSession ds = _back.CreateDrawingSession();

        using namespace Microsoft::UI::Xaml::Controls;
        using namespace Microsoft::UI::Xaml::Media;
        Brush backBrush{ splitView().PaneBackground() };
        SolidColorBrush scbBack = backBrush.try_as<SolidColorBrush>();
        Windows::UI::Color colorBack{scbBack.Color()};

        ds.FillRectangle(0, 0, width, height, Colors::WhiteSmoke());

        if (singlerenderer)
        {
            // render in one thread
            auto drawinto0 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(0), static_cast<uint16_t>(board.Height()), _back.Size().Width);
            drawinto0.wait();
        }
        else
        {
            if (_boardwidth < 100000)
            {
                // render in 4 threads
                auto drawinto1 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(0), static_cast<uint16_t>(board.Height() * 1 / 4), _back.Size().Width);
                auto drawinto2 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 1 / 4), static_cast<uint16_t>(board.Height() * 1 / 2), _back.Size().Width);
                auto drawinto3 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 1 / 2), static_cast<uint16_t>(board.Height() * 3 / 4), _back.Size().Width);
                auto drawinto4 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 3 / 4), static_cast<uint16_t>(board.Height()), _back.Size().Width);

                drawinto1.wait();
                drawinto2.wait();
                drawinto3.wait();
                drawinto4.wait();
            }
            else
            {
                // render in 8 threads
                auto drawinto1 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(0), static_cast<uint16_t>(board.Height() * 1 / 8), _back.Size().Width);
                auto drawinto2 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 1 / 8), static_cast<uint16_t>(board.Height() * 2 / 8), _back.Size().Width);
                auto drawinto3 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 2 / 8), static_cast<uint16_t>(board.Height() * 3 / 8), _back.Size().Width);
                auto drawinto4 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 3 / 8), static_cast<uint16_t>(board.Height() * 4 / 8), _back.Size().Width);
                auto drawinto5 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 4 / 8), static_cast<uint16_t>(board.Height() * 5 / 8), _back.Size().Width);
                auto drawinto6 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 5 / 8), static_cast<uint16_t>(board.Height() * 6 / 8), _back.Size().Width);
                auto drawinto7 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 6 / 8), static_cast<uint16_t>(board.Height() * 7 / 8), _back.Size().Width);
                auto drawinto8 = std::async(&MainWindow::DrawInto, this, std::ref(ds), static_cast<uint16_t>(board.Height() * 7 / 8), static_cast<uint16_t>(board.Height()), _back.Size().Width);

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
            //using namespace Microsoft::UI::Xaml::Controls;
            //GoButton().Icon(SymbolIcon(Symbol::Play));
            //GoButton().Label(L"Play");
            StartGameLoop();

            m_propertyChanged(*this, PropertyChangedEventArgs{ L"BoardWidth" });
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
        std::wstring strTitle = std::format(L"Generation\r\nAlive\r\nTotal Cells");
        std::wstring strContent = std::format(L"{:8}\r\n{:8}\r\n{:8}", board.Generation(), board.GetLiveCount(), board.GetSize());
        sender.Invalidate();
        
        // draw the text left aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Left);
        args.DrawingSession().DrawText(strTitle, 0, 0, 160, 100, colorText, canvasFmt);

        // draw the values right aligned
        canvasFmt.HorizontalAlignment(Microsoft::Graphics::Canvas::Text::CanvasHorizontalAlignment::Right);
        args.DrawingSession().DrawText(strContent, 0, 0, 160, 100, colorText, canvasFmt);
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

    Windows::UI::Color MainWindow::HSVtoRGB2(float H, float S, float V)
    {
        float s = S / 100.0f;
        float v = V / 100.0f;
        float C = s * v;
        float X = static_cast<float>(C * (1.0f - abs(fmod(H / 60.0, 2.0f) - 1.0f)));
        float m = v - C;
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;

        if (H >= 0 && H < 60) {
            r = C, g = X, b = 0;
        }
        else if (H >= 60 && H < 120) {
            r = X, g = C, b = 0;
        }
        else if (H >= 120 && H < 180) {
            r = 0, g = C, b = X;
        }
        else if (H >= 180 && H < 240) {
            r = 0, g = X, b = C;
        }
        else if (H >= 240 && H < 300) {
            r = X, g = 0, b = C;
        }
        else {
            r = C, g = 0, b = X;
        }
        uint8_t R = static_cast<uint8_t>((r + m) * 255);
        uint8_t G = static_cast<uint8_t>((g + m) * 255);
        uint8_t B = static_cast<uint8_t>((b + m) * 255);
    
        return ColorHelper::FromArgb(255, R, G, B);
    }
}




