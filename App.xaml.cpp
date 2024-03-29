﻿#include "pch.h"

#include "App.xaml.h"

#include <iostream>
#include <locale>

#include "MainWindow.xaml.h"

#include "Log.h"

using namespace winrt;
using namespace ModernLife;
using namespace ModernLife::implementation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    // make sure Windows and C++ runtime are set for utf8
    auto UTF8 = std::locale("en_US.UTF-8");
    std::locale::global(UTF8);
    std::cout.imbue(UTF8);
    setlocale(LC_ALL, "en_us.utf8");
    
    //InitializeComponent(); https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const&, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

App::~App()
{
}

/// <summary>
/// Invoked when the application is launched.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&)
{
    window = make<MainWindow>();
    window.Activate();
}
