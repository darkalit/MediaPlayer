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
        void PlayPauseIconSource(hstring const& value);

        bool ControlsEnabled();

        Microsoft::UI::Xaml::Input::ICommand PlayPause();
        Microsoft::UI::Xaml::Input::ICommand ResizeVideo();
        Microsoft::UI::Xaml::Input::ICommand SetSwapChain();
        Microsoft::UI::Xaml::Input::ICommand DefaultMode();

    private:
        MediaPlayer::PlayerService m_PlayerService;

        hstring m_PlayPauseIconSource = L"ms-appx:///Assets/PlayIcon.png";

        Microsoft::UI::Xaml::Input::ICommand m_PlayPauseCommand = nullptr;
        Microsoft::UI::Xaml::Input::ICommand m_ResizeVideoCommand = nullptr;
        Microsoft::UI::Xaml::Input::ICommand m_SetSwapChainCommand = nullptr;
        Microsoft::UI::Xaml::Input::ICommand m_DefaultModeCommand = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct PiPWindowViewModel : PiPWindowViewModelT<PiPWindowViewModel, implementation::PiPWindowViewModel>
    {
    };
}
