#pragma once

#include "RecorderWindow.g.h"
#include "ViewModels/RecorderViewModel.h"

namespace winrt::MediaPlayer::implementation
{
    struct RecorderWindow : RecorderWindowT<RecorderWindow>
    {
        RecorderWindow();

        MediaPlayer::RecorderViewModel ViewModel();
    private:
        MediaPlayer::RecorderViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct RecorderWindow : RecorderWindowT<RecorderWindow, implementation::RecorderWindow>
    {
    };
}
