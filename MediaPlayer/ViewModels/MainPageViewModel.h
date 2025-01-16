#pragma once
#include "MainPageViewModel.g.h"
#include "Framework/DelegateCommand.h"
#include "Framework/BindableBase.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct MainPageViewModel : MainPageViewModelT<MainPageViewModel, MediaPlayer::implementation::BindableBase>
    {
        MainPageViewModel();

        hstring Title();
        hstring AuthorAlbum();
        hstring VolumeText();
        double VolumeValue();
        void VolumeValue(double value);
        hstring CurrentTimeText();
        hstring RemainingTimeText();
        double CurrentTimeValue();
        void CurrentTimeValue(double value);
        double DurationValue();
        hstring PlayPauseIconSource();
        void PlayPauseIconSource(hstring const& value);
        bool ControlsEnabled();

        winrt::Microsoft::UI::Xaml::Input::ICommand ResizeVideo();
        winrt::Microsoft::UI::Xaml::Input::ICommand SetSwapChain();
        winrt::Microsoft::UI::Xaml::Input::ICommand OpenPlaylists();
        winrt::Microsoft::UI::Xaml::Input::ICommand PlayPause();
        winrt::Microsoft::UI::Xaml::Input::ICommand Play();
        winrt::Microsoft::UI::Xaml::Input::ICommand Pause();
        winrt::Microsoft::UI::Xaml::Input::ICommand Next();
        winrt::Microsoft::UI::Xaml::Input::ICommand Prev();

    private:
        void ElapsedTimeHandler(Microsoft::UI::Dispatching::DispatcherQueueTimer const& sender, Windows::Foundation::IInspectable const& args);

        MediaPlayer::PlayerService m_PlayerService;
        Microsoft::UI::Dispatching::DispatcherQueueTimer m_Timer;

        double m_CurrentTime = 0.0;
        hstring m_PlayPauseIconSource = L"ms-appx:///Assets/PlayIcon.png";

        MediaPlayer::DelegateCommand m_ResizeVideoCommand = nullptr;
        MediaPlayer::DelegateCommand m_SetSwapChainCommand = nullptr;
        MediaPlayer::DelegateCommand m_OpenPlaylistsCommand = nullptr;
        MediaPlayer::DelegateCommand m_PlayPauseCommand = nullptr;
        MediaPlayer::DelegateCommand m_PlayCommand = nullptr;
        MediaPlayer::DelegateCommand m_PauseCommand = nullptr;
        MediaPlayer::DelegateCommand m_NextCommand = nullptr;
        MediaPlayer::DelegateCommand m_PrevCommand = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct MainPageViewModel : MainPageViewModelT<MainPageViewModel, implementation::MainPageViewModel>
    {
    };
}
