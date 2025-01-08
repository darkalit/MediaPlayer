#pragma once

#include "App.xaml.g.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        static std::shared_ptr<PlayerService> GetPlayerService();
        static HWND GetMainWindow();

    private:
        static std::shared_ptr<PlayerService> m_PlayerService;

        static winrt::Microsoft::UI::Xaml::Window s_Window;
    };
}
