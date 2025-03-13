#include "pch.h"
#include "PiPWindowViewModel.h"
#include "PiPWindowViewModel.g.cpp"

#include "App.xaml.h"

namespace winrt::MediaPlayer::implementation
{
    PiPWindowViewModel::PiPWindowViewModel()
    {
        m_DefaultModeCommand = make<DelegateCommand>([this](auto&&)
        {
           App::OpenMainWindow(); 
        });
    }

    hstring PiPWindowViewModel::PlayPauseIconSource()
    {
        return m_PlayPauseIconSource;
    }

    winrt::Microsoft::UI::Xaml::Input::ICommand PiPWindowViewModel::DefaultMode()
    {
        return m_DefaultModeCommand;
    }
}
