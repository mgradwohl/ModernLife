// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include<future>
#include<format>
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
constexpr int maxage = 1000;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ModernLife::implementation
{
    constexpr bool drawgrid = false;
    constexpr bool drawstats = true;

    MainWindow::MainWindow()
    {
        InitializeComponent();

        StartGameLoop();
    }

    void MainWindow::OnTick(IInspectable const& sender, IInspectable const& event)
    {
        auto conway = std::async(&Board::ConwayUpdateBoardWithNextState, &board);
        conway.wait();

        auto nextgen = std::async(&Board::ApplyNextStateToBoard, &board);
        nextgen.wait();
        theCanvas().Invalidate();
    }

    void MainWindow::StartGameLoop()
    {
        // create the board
        board = Board{ cellcount, cellcount };

        // add a random population
        auto randomizer = std::async(&Board::RandomizeBoard, &board, 0.4f);
        randomizer.wait();

        // create and start a timer
        _controller = DispatcherQueueController::CreateOnDedicatedThread();
        _queue = _controller.DispatcherQueue();
        _timer = _queue.CreateTimer();
        using namespace  std::literals::chrono_literals;
        _timer.Interval(std::chrono::milliseconds{ 16 });
        _timer.IsRepeating(true);
        auto registrationtoken = _timer.Tick({ this, &MainWindow::OnTick });

        _timer.Start();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        args.DrawingSession().DrawImage(GetBackBuffer(), 0, 0);
    }

    CanvasRenderTarget& MainWindow::GetBackBuffer()
    {
        std::scoped_lock lock { lockbackbuffer };

        return _back;
    }

    Windows::UI::Color MainWindow::GetCellColor(const Cell& cell)
    {
        uint8_t colorscale = 0;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;

        colorscale = (cell.Age() * 255) / maxage;
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

    void MainWindow::DrawInto(CanvasDrawingSession& ds, float width, float height)
	{
		ds.Clear(Colors::WhiteSmoke());

        float inc = width / cellcount;
        if (drawgrid)
		{
			for (int i = 0; i <= cellcount; i++)
			{
				ds.DrawLine(0, i * inc, height, i * inc, Colors::DarkSlateGray());
				ds.DrawLine(i * inc, 0, i * inc, width, Colors::DarkSlateGray());
			}
		}

        float w = (width / cellcount) - 2;
		float posx = 1.0f;
		float posy = 1.0f;

        for (int y = 0; y < cellcount; y++)
		{
			for (int x = 0; x < cellcount; x++)
			{
				if (const Cell& cell = board.GetCell(x, y); cell.IsAlive())
				{
                    Windows::UI::Color cellcolor = GetCellColor(cell);
                    ds.DrawRoundedRectangle(posx, posy, w, w, 2, 2, cellcolor);
				}
				posx += w;
			}
			posy += w;
			posx = 1.0f;
		}
	}

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

        if (!board.IsDirty())
            return;

        // if the back buffer doesn't exist or is the wrong size, create it
        if (nullptr == _back || _back.Size() != sender.Size())
        {
            constexpr int bestsize = cellcount * 4;
            winrt::Windows::Foundation::Size huge = sender.Size();
            float width = max(huge.Width, bestsize);
            float height = max(huge.Height, bestsize);

            CanvasDevice device = CanvasDevice::GetSharedDevice();
            {
                std::scoped_lock lock{ lockbackbuffer };
                _back = CanvasRenderTarget(device, width, height, sender.Dpi());
            }
        }

        CanvasDrawingSession ds = _back.CreateDrawingSession();
        auto drawinto = std::async(&MainWindow::DrawInto, this, std::ref(ds), _back.Size().Width, _back.Size().Height);
        drawinto.wait();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::theCanvasDebug_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        args.DrawingSession().Clear(Colors::WhiteSmoke());
        std::wstring str{L"Modern Life\0"};
        if (drawstats)
        {
            str = std::format(L"Modern Life\r\nGeneration {}\r\nAlive {}\r\nTotal Cells {}\r\n\0", board.Generation(), board.GetLiveCount(), board.GetSize());
            sender.Invalidate();
        }

        args.DrawingSession().DrawTextW(str, 0, 0, Colors::Black());
    }
}


