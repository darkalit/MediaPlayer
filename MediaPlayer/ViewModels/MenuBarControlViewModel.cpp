#include "pch.h"
#include "MenuBarControlViewModel.h"
#include "MenuBarControlViewModel.g.cpp"

#include "App.xaml.h"
#include "winrt/Windows.Storage.Pickers.h"
#include "winrt/Windows.Foundation.h"
#include "ShObjIdl.h"
#include "Utils.h"

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Foundation;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;

namespace winrt::MediaPlayer::implementation
{
    MenuBarControlViewModel::MenuBarControlViewModel()
        : m_PlayerService(App::GetPlayerService())
    {
        m_ChangePlaybackSpeedCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            if (!m_PlayerService.HasSource()) return;

            auto speed = unbox_value<double>(parameter);

            m_PlayerService.PlaybackSpeed(speed);
        });

        m_ExitCommand = make<DelegateCommand>([&](auto&&)
        {
            m_PlayerService.Stop();
            Application::Current().Exit();
        });

        m_OpenFilesCommand = make<DelegateCommand>([&](auto&&) -> fire_and_forget
        {
            auto files = co_await OpenFilePickerAsync();

            for (const auto& file : files)
            {
                m_PlayerService.AddSource(file.Path(), file.DisplayName());
            }
        });
    }

    IAsyncOperation<Collections::IVectorView<StorageFile>> MenuBarControlViewModel::OpenFilePickerAsync()
    {
        FileOpenPicker filePicker{};
        filePicker.as<IInitializeWithWindow>()->Initialize(App::GetMainWindow());
        filePicker.SuggestedStartLocation(PickerLocationId::Desktop);
        filePicker.FileTypeFilter().ReplaceAll({
            L".3g2", L".3gp", L".3gp2", L".3gpp",
            L".asf", L".wma", L".wmv",
            L".aac", L".adts",
            L".avi",
            L".mp3",
            L".m4a", L".m4v", L".mov", L".mp4",
            L".sami", L".smi",
            L".wav"
            });

        co_return co_await filePicker.PickMultipleFilesAsync();
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::ChangePlaybackSpeed()
    {
        return m_ChangePlaybackSpeedCommand;
    }
    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::Exit()
    {
        return m_ExitCommand;
    }
    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::OpenFiles()
    {
        return m_OpenFilesCommand;
    }
}
