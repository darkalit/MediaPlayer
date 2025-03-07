#pragma once
#include "MenuBarControlViewModel.g.h"
#include "Framework/BindableBase.h"
#include "Framework/DelegateCommand.h"
#include "Services/PlayerService.h"
#include "Pages/RecorderWindow.xaml.h"

namespace winrt::MediaPlayer::implementation
{
    struct MenuBarControlViewModel : MenuBarControlViewModelT<MenuBarControlViewModel, MediaPlayer::implementation::BindableBase>
    {
        MenuBarControlViewModel();

        winrt::Windows::Foundation::Collections::IObservableVector<SubtitleStream> SubTracks();

        Microsoft::UI::Xaml::Input::ICommand CreateSnapshotFile();
        Microsoft::UI::Xaml::Input::ICommand ChangePlaybackMode();
        Microsoft::UI::Xaml::Input::ICommand ChangePlaybackSpeed();
        Microsoft::UI::Xaml::Input::ICommand ChangeSubTrack();
        Microsoft::UI::Xaml::Input::ICommand Exit();
        Microsoft::UI::Xaml::Input::ICommand OpenFiles();
        Microsoft::UI::Xaml::Input::ICommand OpenSubtitle();
        Microsoft::UI::Xaml::Input::ICommand OpenRecorderWindow();

    private:
        MediaPlayer::PlayerService m_PlayerService;
        MediaPlayer::DelegateCommand m_CreateSnapshotFileCommand = nullptr;
        MediaPlayer::DelegateCommand m_ChangePlaybackModeCommand = nullptr;
        MediaPlayer::DelegateCommand m_ChangePlaybackSpeedCommand = nullptr;
        MediaPlayer::DelegateCommand m_ChangeSubTrackCommand = nullptr;
        MediaPlayer::DelegateCommand m_ExitCommand = nullptr;
        MediaPlayer::DelegateCommand m_OpenFilesCommand = nullptr;
        MediaPlayer::DelegateCommand m_OpenSubtitleCommand = nullptr;
        MediaPlayer::DelegateCommand m_OpenRecorderWindowCommand = nullptr;

        MediaPlayer::RecorderWindow m_RecorderWindow = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct MenuBarControlViewModel : MenuBarControlViewModelT<MenuBarControlViewModel, implementation::MenuBarControlViewModel>
    {
    };
}
