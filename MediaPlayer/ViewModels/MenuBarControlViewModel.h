#pragma once
#include "MenuBarControlViewModel.g.h"
#include "Framework/BindableBase.h"
#include "Framework/DelegateCommand.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct MenuBarControlViewModel : MenuBarControlViewModelT<MenuBarControlViewModel, MediaPlayer::implementation::BindableBase>
    {
        MenuBarControlViewModel();

        Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<Windows::Storage::StorageFile>> OpenFilePickerAsync();

        Microsoft::UI::Xaml::Input::ICommand CreateSnapshotFile();
        Microsoft::UI::Xaml::Input::ICommand ChangePlaybackMode();
        Microsoft::UI::Xaml::Input::ICommand ChangePlaybackSpeed();
        Microsoft::UI::Xaml::Input::ICommand Exit();
        Microsoft::UI::Xaml::Input::ICommand OpenFiles();

    private:
        MediaPlayer::PlayerService m_PlayerService;
        MediaPlayer::DelegateCommand m_CreateSnapshotFileCommand = nullptr;
        MediaPlayer::DelegateCommand m_ChangePlaybackModeCommand = nullptr;
        MediaPlayer::DelegateCommand m_ChangePlaybackSpeedCommand = nullptr;
        MediaPlayer::DelegateCommand m_ExitCommand = nullptr;
        MediaPlayer::DelegateCommand m_OpenFilesCommand = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct MenuBarControlViewModel : MenuBarControlViewModelT<MenuBarControlViewModel, implementation::MenuBarControlViewModel>
    {
    };
}
