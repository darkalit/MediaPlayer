#pragma once

#include "App.xaml.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
        void OnNavigationFailed(const IInspectable, const Microsoft::UI::Xaml::Navigation::INavigationFailedEventArgs e);

        static HWND GetMainWindow();

    private:
        Microsoft::UI::Xaml::Controls::Frame CreateRootFrame();

        static winrt::Microsoft::UI::Xaml::Window s_Window;
    };
}
