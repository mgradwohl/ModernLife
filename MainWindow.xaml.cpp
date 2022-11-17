// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

constexpr int cellsperrow = 50;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ModernLife::implementation
{
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
        winrt::Windows::Foundation::Size huge = sender.Size();
        float inc = huge.Width / cellsperrow;

        for (int i = 0; i < cellsperrow; i++)
        {
            args.DrawingSession().DrawLine(0, i* inc, huge.Height, i * inc, Colors::DarkSlateGray());
            args.DrawingSession().DrawLine(i * inc, 0, i * inc, huge.Width, Colors::DarkSlateGray());
        }

        float w = (huge.Width / cellsperrow) - 2;
        args.DrawingSession().DrawRectangle(1, 1, w, w, Colors::Red());
        args.DrawingSession().DrawRoundedRectangle(inc+1, inc+1, w, w, 2, 2, Colors::Blue());
    }

}
