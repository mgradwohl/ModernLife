// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ModernLife::implementation
{
    bool drawgrid = false;
    constexpr int cellcount = 50;
    CanvasDevice device = CanvasDevice::GetSharedDevice();
    CanvasRenderTarget back(device, 800, 800, 96);

    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        args.DrawingSession().DrawImage(back, 0, 0);
    }

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm
        CanvasDrawingSession ds = back.CreateDrawingSession();
        ds.Clear(Colors::Black());

        winrt::Windows::Foundation::Size huge = sender.Size();
        float inc = huge.Width / cellcount;

        if (drawgrid)
        {
            for (int i = 0; i <= cellcount; i++)
            {
                ds.DrawLine(0, i * inc, huge.Height, i * inc, Colors::DarkSlateGray());
                ds.DrawLine(i * inc, 0, i * inc, huge.Width, Colors::DarkSlateGray());
            }
        }

        float w = (huge.Width / cellcount) - 2;
        ds.DrawRoundedRectangle(1, 1, w, w, 2, 2, Colors::Red());
        ds.DrawRoundedRectangle(inc+1, inc+1, w, w, 2, 2, Colors::Blue());

        /*
        An app can close, and re-open drawing sessions on a CanvasRenderTarget abitrarily many times.
        Drawing operations are not committed to the CanvasRenderTarget until the drawing session object is disposed. In C#, a 'using' block can organize this.
        It's worth pointing out that CanvasRenderTarget is not a XAML control, and does not involve the XAML tree at all. It is suitable for both XAML and non-XAML-based apps.
        */
    }

}
