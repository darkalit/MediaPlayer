#include "pch.h"
#include "MainPageViewModel.h"
#include "MainPageViewModel.g.cpp"

#include "App.xaml.h"
#include "Utils.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI;

namespace winrt::MediaPlayer::implementation
{
    MainPageViewModel::MainPageViewModel()
        : m_PlayerService(App::GetPlayerService())
        , m_Timer(Dispatching::DispatcherQueue::GetForCurrentThread().CreateTimer())

    {
        m_Timer.Interval(std::chrono::milliseconds(200));
        m_Timer.Tick({ this, &MainPageViewModel::ElapsedTimeHandler });
        m_Timer.Start();

        m_ResizeVideoCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto size = unbox_value<Numerics::float2>(parameter);
            m_PlayerService.ResizeVideo(size.x, size.y);
        });

        m_SetSwapChainCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto swapchainpanel = parameter.as<Microsoft::UI::Xaml::Controls::SwapChainPanel>();
            m_PlayerService.SwapChainPanel(swapchainpanel);
        });

        m_OpenPlaylistsCommand = make<DelegateCommand>([&](auto&&)
        {
            App::Navigate<PlaylistPage>();
        });

        m_PlayPauseCommand = make<DelegateCommand>([&](auto&&)
        {
            if (m_PlayerService.State() == PlayerServiceState::STOPPED || m_PlayerService.State() == PlayerServiceState::PAUSED)
            {
                m_PlayerService.Start();
                PlayPauseIconSource(L"ms-appx:///Assets/PauseIcon.png");
            }
            else if (m_PlayerService.State() == PlayerServiceState::PLAYING)
            {
                m_PlayerService.Pause();
                PlayPauseIconSource(L"ms-appx:///Assets/PlayIcon.png");
            }
        });

        m_PlayCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto time = parameter.as<uint64_t>();
            CurrentTimeValue(time / 1000.0);
            m_PlayerService.Start(time);
            PlayPauseIconSource(L"ms-appx:///Assets/PauseIcon.png");
        });

        m_PauseCommand = make<DelegateCommand>([&](auto&&)
        {
            m_PlayerService.Pause();
            PlayPauseIconSource(L"ms-appx:///Assets/PlayIcon.png");
        });

        m_NextCommand = make<DelegateCommand>([&](auto&&)
        {
            m_PlayerService.Next();
        });

        m_PrevCommand = make<DelegateCommand>([&](auto&&)
        {
            m_PlayerService.Prev();
        });

        m_PiPModeCommand = make<DelegateCommand>([&](auto&&)
        {
           App::OpenPiPWindow();
        });
    }

    hstring MainPageViewModel::Path()
    {
        return m_PlayerService.Metadata().Path;
    }

    hstring MainPageViewModel::Title()
    {
        auto title = m_PlayerService.Metadata().Title;
        if (title.empty())
        {
            return L"Open Playlists";
        }

        return title;
    }

    hstring MainPageViewModel::AuthorAlbum()
    {
        auto author = m_PlayerService.Metadata().Author;
        auto album = m_PlayerService.Metadata().AlbumTitle;

        std::wstring res;

        res += author;

        if (!album.empty())
        {
            res += L" - " + album;
        }

        return res.c_str();
    }

    hstring MainPageViewModel::VolumeText()
    {
        return to_hstring(VolumeValue()) + L"%";
    }

    double MainPageViewModel::VolumeValue()
    {
        return std::round(m_PlayerService.Volume() * 100.0);
    }

    void MainPageViewModel::VolumeValue(double value)
    {
        if (VolumeValue() != value)
        {
            m_PlayerService.Volume(value / 100.0);
            RaisePropertyChanged(L"VolumeValue");
            RaisePropertyChanged(L"VolumeText");
        }        
    }

    hstring MainPageViewModel::CurrentTimeText()
    {
        return Utils::DurationToString(CurrentTimeValue() * 1000.0);
    }

    hstring MainPageViewModel::RemainingTimeText()
    {
        return Utils::DurationToString((DurationValue() - CurrentTimeValue()) * 1000.0);
    }

    double MainPageViewModel::CurrentTimeValue()
    {
        return m_CurrentTime;
    }

    void MainPageViewModel::CurrentTimeValue(double value)
    {
        if (m_CurrentTime != value)
        {
            m_CurrentTime = value;
            RaisePropertyChanged(L"CurrentTimeValue");
        }
    }

    double MainPageViewModel::DurationValue()
    {
        return m_PlayerService.Metadata().Duration / 1000.0;
    }

    hstring MainPageViewModel::PlayPauseIconSource()
    {
        return m_PlayPauseIconSource;
    }

    void MainPageViewModel::PlayPauseIconSource(hstring const& value)
    {
        if (m_PlayPauseIconSource != value)
        {
            m_PlayPauseIconSource = value;
            RaisePropertyChanged(L"PlayPauseIconSource");
        }
    }

    bool MainPageViewModel::ControlsEnabled()
    {
        return m_PlayerService.HasSource();
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::ResizeVideo()
    {
        return m_ResizeVideoCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::SetSwapChain()
    {
        return m_SetSwapChainCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::OpenPlaylists()
    {
        return m_OpenPlaylistsCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::PlayPause()
    {
        return m_PlayPauseCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::Play()
    {
        return m_PlayCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::Pause()
    {
        return m_PauseCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::Next()
    {
        return m_NextCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::Prev()
    {
        return m_PrevCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand MainPageViewModel::PiPMode()
    {
        return m_PiPModeCommand;
    }

    void MainPageViewModel::ElapsedTimeHandler(Microsoft::UI::Dispatching::DispatcherQueueTimer const& sender, Windows::Foundation::IInspectable const& args)
    {
        RaisePropertyChanged(L"Title");
        RaisePropertyChanged(L"AuthorAlbum");
        if (m_PlayerService.State() == PlayerServiceState::PLAYING)
        {
            CurrentTimeValue(m_PlayerService.Position() / 1000.0);
        }        
        RaisePropertyChanged(L"CurrentTimeText");
        RaisePropertyChanged(L"RemainingTimeText");
        RaisePropertyChanged(L"DurationValue");
        RaisePropertyChanged(L"ControlsEnabled");
    }
}
