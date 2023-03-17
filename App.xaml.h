#pragma once

#include "App.xaml.g.h"

#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::ModernLife::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
