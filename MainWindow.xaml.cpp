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

        board = Board{ cellcount, cellcount };

        auto randomizer = std::async(&Board::RandomizeBoard, &board, 0.4f);
        randomizer.wait();

        CanvasDevice device = CanvasDevice::GetSharedDevice();
        _back = GetBackBuffer();
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
				const Cell& cell = board.GetCell(x, y);
				if (cell.IsAlive())
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

        winrt::Windows::Foundation::Size huge = sender.Size();
        float width = max(huge.Width, 5000);
        float height = max(huge.Height, 5000);

        CanvasDevice device = CanvasDevice::GetSharedDevice();
        CanvasRenderTarget flip{ device, width, height, sender.Dpi() };
        CanvasDrawingSession ds = flip.CreateDrawingSession();

        auto drawinto = std::async(&MainWindow::DrawInto, this, std::ref(ds), huge.Width, huge.Height);
        drawinto.wait();

        {
            // resize the back buffer
            std::scoped_lock lock{ lockbackbuffer };
            _back = flip;
        }
        sender.Invalidate();

        auto C = std::bind_front(&Board::ConwayRules, &board);
        board.UpdateBoardWithNextState(C);

        auto nextgen = std::async(&Board::ApplyNextStateToBoard, &board);
        nextgen.wait();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }
}


void winrt::ModernLife::implementation::MainWindow::theCanvasDebug_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
{
    std::wstring str{L"Modern Life\0"};
    if (drawstats)
    {
        str = std::format(L"Modern Life\r\nGeneration {}\r\nAlive {}\r\n\0", board.Generation(), board.GetLiveCount());
        sender.Invalidate();
    }

    args.DrawingSession().DrawTextW(str, 0, 0, Colors::Black());
}
