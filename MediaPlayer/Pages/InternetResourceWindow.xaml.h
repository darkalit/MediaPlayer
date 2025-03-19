#pragma once

#include "InternetResourceWindow.g.h"
#include "ViewModels/InternetResourceViewModel.h"

namespace winrt::MediaPlayer::implementation
{
    struct InternetResourceWindow : InternetResourceWindowT<InternetResourceWindow>
    {
        InternetResourceWindow();

        MediaPlayer::InternetResourceViewModel ViewModel();

    private:
        MediaPlayer::InternetResourceViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct InternetResourceWindow : InternetResourceWindowT<InternetResourceWindow, implementation::InternetResourceWindow>
    {
    };
}
