#pragma once

#include "PiPWindow.g.h"
#include "ViewModels/PiPWindowViewModel.h"

namespace winrt::MediaPlayer::implementation
{
    struct PiPWindow : PiPWindowT<PiPWindow>
    {
        PiPWindow();

        MediaPlayer::PiPWindowViewModel ViewModel();

    private:
        MediaPlayer::PiPWindowViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct PiPWindow : PiPWindowT<PiPWindow, implementation::PiPWindow>
    {
    };
}
