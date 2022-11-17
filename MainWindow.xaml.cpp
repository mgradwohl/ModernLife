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

        for (int i = 0; i < 100; i++)
        {
            float inc = i * huge.Width / 100;
            args.DrawingSession().DrawLine(0, inc, huge.Height, inc, Colors::DarkSlateGray());
        }

        for (int i = 0; i < 100; i++)
        {
            float inc = i * huge.Height / 100;
            args.DrawingSession().DrawLine(inc, 0, inc, huge.Width, Colors::DarkSlateGray());
        }
    }

}
