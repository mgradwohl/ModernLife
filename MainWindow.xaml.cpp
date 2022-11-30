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
    //auto C = std::bind_front(&Board::ConwayRules, &board);
    //auto D = std::bind_front(&Board::DayAndNightRules, &board);
    //auto S = std::bind_front(&Board::SeedsRules, &board);
    //auto B = std::bind_front(&Board::BriansBrainRules, &board);
    //auto H = std::bind_front(&Board::HighlifeRules, &board);
    //auto L = std::bind_front(&Board::LifeWithoutDeathRules, &board);
    bool drawgrid = false;

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
                    //ds.FillRoundedRectangle(posx, posy, w, w, 2, 2, cellcolor);
                    //std::wstring str = std::format(L"Generation {} \0", board.Generation());
                    //ds.DrawTextW(str, 0, 0, Colors::Black());
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
        board.UpdateBoard(C);

        auto nextgen = std::async(&Board::NextGeneration, &board);
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
