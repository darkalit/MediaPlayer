#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "App.xaml.h"
#include "Services/PlayerService.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;
using namespace Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    MainPage::MainPage()
        : m_TimelineDispatcherTimer(Dispatching::DispatcherQueue::GetForCurrentThread().CreateTimer())
        , m_PlayerService(App::GetPlayerService())
    {
        m_TimelineDispatcherTimer.Interval(std::chrono::milliseconds(100));
        m_TimelineDispatcherTimer.Tick([this](auto const&, auto const&) {
            this->UpdateUI();
        });
        m_TimelineDispatcherTimer.Start();
    }

    void MainPage::OnLoad(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        Slider_Timeline().AddHandler(
            UIElement::PointerPressedEvent(),
            box_value(PointerEventHandler{ this, &MainPage::Slider_Timeline_PointerPressed }),
            true);
        Slider_Timeline().AddHandler(
            UIElement::PointerReleasedEvent(),
            box_value(PointerEventHandler{ this, &MainPage::Slider_Timeline_PointerReleased }),
            true);
        Slider_Timeline().AddHandler(
            UIElement::PointerMovedEvent(),
            box_value(PointerEventHandler{ this, &MainPage::Slider_Timeline_PointerMoved }),
            true);

        m_PlayerService->SetSwapChainPanel(SwapChainPanel_Video());
    }

    void MainPage::OnUnload(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->UnsetSwapChainPanel();
    }

    void MainPage::Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        UpdateUI();
    }

    void MainPage::SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, SizeChangedEventArgs const&)
    {
        auto size = SwapChainPanel_Video().ActualSize();
        m_PlayerService->ResizeVideo(size.x, size.y);
    }

    void MainPage::Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        long long time = m_PlayerService->GetMetadata().Duration * (Slider_Timeline().Value() / Slider_Timeline().Maximum());
        m_PlayerService->Start(time);
        UpdateUI();
    }


    void MainPage::Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        m_PlayerService->Pause();
        UpdateUI();
    }


    void MainPage::Button_PlayPause_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        if (m_PlayerService->GetState() == PlayerService::State::STOPPED || m_PlayerService->GetState() == PlayerService::State::PAUSED)
        {
            m_PlayerService->Start();
        }
        else if (m_PlayerService->GetState() == PlayerService::State::PLAYING)
        {
            m_PlayerService->Pause();
        }

        UpdateUI();
    }

    void MainPage::Slider_Volume_PointerMoved(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        double volume = Slider_Volume().Value();
        m_PlayerService->SetVolume(volume / 100.0);
        TextBlock_Volume().Text(to_hstring(volume) + L"%");
    }

    void MainPage::Button_Prev_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->Prev();
    }


    void MainPage::Button_Next_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->Next();
    }

    void MainPage::Button_Playlist_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        App::Navigate<PlaylistPage>();
    }


    void MainPage::UpdateUI()
    {
        auto metadata = m_PlayerService->GetMetadata();

        std::wstring title = L"";
        std::wstring authorAlbum = L"";

        title = metadata.Title;
        authorAlbum += metadata.Author;
        authorAlbum += (!metadata.Author.empty() ? L" - " : L"") + metadata.AlbumTitle;

        if (title.empty())
        {
            title = L"Playlist";
        }

        TextBlock_Title().Text(title);
        TextBlock_AuthorAlbum().Text(authorAlbum);

        if (metadata.Duration)
        {
            Slider_Timeline().Maximum(metadata.Duration / 1000.0);
        }

        UpdateTimeline();

        if (m_PlayerService->GetState() == PlayerService::State::STOPPED || m_PlayerService->GetState() == PlayerService::State::PAUSED)
        {
            BitmapImage_PlayPause().UriSource(Uri{ L"ms-appx:///Assets/PlayIcon.png" });
        }
        else if (m_PlayerService->GetState() == PlayerService::State::PLAYING)
        {
            BitmapImage_PlayPause().UriSource(Uri{ L"ms-appx:///Assets/PauseIcon.png" });
        }
    }
    void MainPage::UpdateTimeline()
    {
        Slider_Timeline().IsEnabled(m_PlayerService->HasSource());

        if (!m_PlayerService->HasSource()) return;

        if (m_PlayerService->GetState() == PlayerService::State::PLAYING)
        {
            double progress = static_cast<double>(m_PlayerService->GetPosition()) / 1000.0;
            Slider_Timeline().Value(progress);
        }
        TextBlock_Position().Text(m_PlayerService->DurationToWString(Slider_Timeline().Value() * 1000.0));
        TextBlock_RemainingTime().Text(m_PlayerService->DurationToWString((Slider_Timeline().Maximum() - Slider_Timeline().Value()) * 1000.0));
    }
}
