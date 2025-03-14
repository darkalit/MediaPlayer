#include "pch.h"
#include "PiPWindowViewModel.h"
#include "PiPWindowViewModel.g.cpp"

#include "Services/PlayerService.h"
#include "App.xaml.h"

using namespace winrt;
using namespace Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    PiPWindowViewModel::PiPWindowViewModel()
        : m_PlayerService(App::GetPlayerService())
    {
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

        m_DefaultModeCommand = make<DelegateCommand>([this](auto&&)
        {
           App::OpenMainWindow(); 
        });
    }

    hstring PiPWindowViewModel::PlayPauseIconSource()
    {
        return m_PlayPauseIconSource;
    }

    void PiPWindowViewModel::PlayPauseIconSource(hstring const& value)
    {
        if (m_PlayPauseIconSource != value)
        {
            m_PlayPauseIconSource = value;
            RaisePropertyChanged(L"PlayPauseIconSource");
        }
    }

    bool PiPWindowViewModel::ControlsEnabled()
    {
        return m_PlayerService.HasSource();
    }

    Microsoft::UI::Xaml::Input::ICommand PiPWindowViewModel::PlayPause()
    {
        return m_PlayPauseCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand PiPWindowViewModel::ResizeVideo()
    {
        return m_ResizeVideoCommand;
    }

    Microsoft::UI::Xaml::Input::ICommand PiPWindowViewModel::SetSwapChain()
    {
        return m_SetSwapChainCommand;
    }

    winrt::Microsoft::UI::Xaml::Input::ICommand PiPWindowViewModel::DefaultMode()
    {
        return m_DefaultModeCommand;
    }
}
