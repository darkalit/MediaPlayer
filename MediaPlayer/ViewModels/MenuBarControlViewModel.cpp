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
        m_SetVideoEffectCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto s = unbox_value<hstring>(parameter);
            m_PlayerService.SetVideoEffect(s);
        });

        m_SetAudioEffectCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto s = unbox_value<hstring>(parameter);
            m_PlayerService.SetAudioEffect(s);
        });

        m_CreateSnapshotFileCommand = make<DelegateCommand>([&](auto&&)
        {
            m_PlayerService.CreateSnapshot();
        });

        m_ChangePlaybackModeCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto s = unbox_value<hstring>(parameter);
            if (s == L"ffmpeg")
            {
                m_PlayerService.Mode(PlayerServiceMode::FFMPEG);
            }
            else if (s == L"mediafoundation")
            {
                m_PlayerService.Mode(PlayerServiceMode::MEDIA_FOUNDATION);
            }
            else if (s == L"auto")
            {
                m_PlayerService.Mode(PlayerServiceMode::AUTO);
            }
        });

        m_ChangePlaybackSpeedCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            if (!m_PlayerService.HasSource()) return;

            auto s = unbox_value<hstring>(parameter);
            wchar_t* endPtr = nullptr;
            double speed = std::wcstod(s.c_str(), &endPtr);

            if (endPtr == s.c_str())
            {
                speed = 1.0;
            }

            m_PlayerService.PlaybackSpeed(speed);
        });

        m_ChangeSubTrackCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            if (!m_PlayerService.HasSource()) return;

            auto index = unbox_value<int32_t>(parameter);

            m_PlayerService.SetSubtitleIndex(index);
        });

        m_ExitCommand = make<DelegateCommand>([&](auto&&)
        {
            m_PlayerService.Stop();
            Application::Current().Exit();
        });

        m_OpenFilesCommand = make<DelegateCommand>([&](auto&&) -> fire_and_forget
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
                L".wav",
                L".flac", L".oga",
                L".ogg", L".ogv", L".opus",
                L".mkv", L".mka", L".mk3d",
                L".flv", L".swf", L".f4v",
                L".webm",
                L".ts", L".m2ts", L".mts",
                L".rm", L".rmvb", L".ra", L".ram",
                L".ac3", L".amr", L".ape", L".au",
                L".dts", L".dvr-ms", L".vob", L".wtv",
                L".mpg", L".mpeg", L".m2v",
                L".dv", L".mxf", L".nut",
                L".srt", L".ass", L".ssa", L".sub",
                L".bmp", L".png", L".jpg", L".jpeg", L".gif"
                });

            auto files = co_await filePicker.PickMultipleFilesAsync();

            for (const auto& file : files)
            {
                m_PlayerService.AddSource(file.Path(), file.DisplayName());
            }
        });

        m_OpenSubtitleCommand = make<DelegateCommand>([&](auto&&) -> fire_and_forget
        {
            FileOpenPicker filePicker{};
            filePicker.as<IInitializeWithWindow>()->Initialize(App::GetMainWindow());
            filePicker.SuggestedStartLocation(PickerLocationId::Desktop);
            filePicker.FileTypeFilter().ReplaceAll({ L".srt", L".ass" });
            auto file = co_await filePicker.PickSingleFileAsync();
            m_PlayerService.SetSubtitleFromFile(file.Path());
        });

        m_OpenRecorderWindowCommand = make<DelegateCommand>([&](auto&&)
        {
            App::OpenRecorderWindow();
        });

        m_OpenInternetResourceWindowCommand = make<DelegateCommand>([&](auto&&)
        {
            App::OpenInternetResourceWindow();
        });
    }

    winrt::Windows::Foundation::Collections::IObservableVector<SubtitleStream> MenuBarControlViewModel::SubTracks()
    {
        return m_PlayerService.SubTracks();
    }

    winrt::Windows::Foundation::Collections::IVector<hstring> MenuBarControlViewModel::VideoEffects()
    {
        return m_PlayerService.VideoEffectNames();
    }

    winrt::Windows::Foundation::Collections::IVector<hstring> MenuBarControlViewModel::AudioEffects()
    {
        return m_PlayerService.AudioEffectNames();
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::SetVideoEffect()
    {
        return m_SetVideoEffectCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::SetAudioEffect()
    {
        return m_SetAudioEffectCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::CreateSnapshotFile()
    {
        return m_CreateSnapshotFileCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::ChangePlaybackMode()
    {
        return m_ChangePlaybackModeCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::ChangePlaybackSpeed()
    {
        return m_ChangePlaybackSpeedCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::ChangeSubTrack()
    {
        return m_ChangeSubTrackCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::Exit()
    {
        return m_ExitCommand;
    }
    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::OpenFiles()
    {
        return m_OpenFilesCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::OpenSubtitle()
    {
        return m_OpenSubtitleCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::OpenRecorderWindow()
    {
        return m_OpenRecorderWindowCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MenuBarControlViewModel::OpenInternetResourceWindow()
    {
        return m_OpenInternetResourceWindowCommand;
    }
}
