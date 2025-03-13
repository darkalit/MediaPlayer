#pragma once
#include "PiPWindowViewModel.g.h"
#include "Framework/DelegateCommand.h"
#include "Framework/BindableBase.h"

namespace winrt::MediaPlayer::implementation
{
    struct PiPWindowViewModel : PiPWindowViewModelT<PiPWindowViewModel, MediaPlayer::implementation::BindableBase>
    {
        PiPWindowViewModel();

        hstring PlayPauseIconSource();

        Microsoft::UI::Xaml::Input::ICommand DefaultMode();

    private:
        hstring m_PlayPauseIconSource = L"ms-appx:///Assets/PlayIcon.png";

        Microsoft::UI::Xaml::Input::ICommand m_DefaultModeCommand = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct PiPWindowViewModel : PiPWindowViewModelT<PiPWindowViewModel, implementation::PiPWindowViewModel>
    {
    };
}
