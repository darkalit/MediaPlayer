#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "App.xaml.h"
#include "winrt/Windows.Storage.Pickers.h"
#include "winrt/Windows.Foundation.h"
#include "ShObjIdl.h"
#include "Services/PlayerService.h"

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Foundation;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;

namespace winrt::MediaPlayer::implementation
{
    MainWindow::MainWindow()
        : m_TimelineDispatcherTimer(Dispatching::DispatcherQueue::GetForCurrentThread().CreateTimer())
        , m_PlayerService(App::GetPlayerService())
    {
        this->Title(L"MediaPlayer");

        m_TimelineDispatcherTimer.Interval(std::chrono::milliseconds(100));
        m_TimelineDispatcherTimer.Tick([this](auto const&, auto const&) {
            this->UpdateUI();
            });
        m_TimelineDispatcherTimer.Start();
    }

    void MainWindow::OnLoad(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        Slider_Timeline().AddHandler(
            UIElement::PointerPressedEvent(),
            box_value(PointerEventHandler{ this, &MainWindow::Slider_Timeline_PointerPressed }),
            true);
        Slider_Timeline().AddHandler(
            UIElement::PointerReleasedEvent(),
            box_value(PointerEventHandler{ this, &MainWindow::Slider_Timeline_PointerReleased }),
            true);
        Slider_Timeline().AddHandler(
            UIElement::PointerMovedEvent(),
            box_value(PointerEventHandler{ this, &MainWindow::Slider_Timeline_PointerMoved }),
            true);
    }    

    IAsyncOperation<Collections::IVectorView<StorageFile>> MainWindow::OpenFilePickerAsync()
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

    fire_and_forget MainWindow::MenuItem_OpenFile_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        auto files = co_await OpenFilePickerAsync();

        for (const auto& file : files)
        {
            m_PlayerService->AddSource(file.Path(), file.DisplayName());
        }
    }

    void MainWindow::MenuItem_Exit_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->Stop();
        Application::Current().Exit();
    }

    void MainWindow::MenuItem_PlaybackSpeed_Click(Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
    {
        Controls::MenuFlyoutItem menuItem;
        sender.as(menuItem);
        if (menuItem.Name() == MenuItem_Speed025().Name())
        {
            m_PlayerService->SetPlaybackSpeed(0.25);
        }
        else if (menuItem.Name() == MenuItem_Speed05().Name())
        {
            m_PlayerService->SetPlaybackSpeed(0.5);
        }
        else if (menuItem.Name() == MenuItem_Speed075().Name())
        {
            m_PlayerService->SetPlaybackSpeed(0.75);
        }
        else if (menuItem.Name() == MenuItem_Speed1().Name())
        {
            m_PlayerService->SetPlaybackSpeed(1.0);
        }
        else if (menuItem.Name() == MenuItem_Speed125().Name())
        {
            m_PlayerService->SetPlaybackSpeed(1.25);
        }
        else if (menuItem.Name() == MenuItem_Speed15().Name())
        {
            m_PlayerService->SetPlaybackSpeed(1.5);
        }
        else if (menuItem.Name() == MenuItem_Speed175().Name())
        {
            m_PlayerService->SetPlaybackSpeed(1.75);
        }
        else if (menuItem.Name() == MenuItem_Speed2().Name())
        {
            m_PlayerService->SetPlaybackSpeed(2.0);
        }
    }

    void MainWindow::Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        long long time = m_PlayerService->GetMetadata().Duration * (Slider_Timeline().Value() / Slider_Timeline().Maximum());
        m_PlayerService->Start(time);
        UpdateUI();
    }


    void MainWindow::Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        m_PlayerService->Pause();
        UpdateUI();
    }


    void MainWindow::Button_PlayPause_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
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

    void MainWindow::Slider_Volume_PointerMoved(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        double volume = Slider_Volume().Value();
        m_PlayerService->SetVolume(volume / 100.0);
        TextBlock_Volume().Text(to_hstring(volume) + L"%");
    }

    void MainWindow::Button_Prev_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->Prev();
        UpdateMediaName();
    }


    void MainWindow::Button_Next_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->Next();
        UpdateMediaName();
    }

    void MainWindow::Button_Playlist_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        auto pageType = PageFrame().CurrentSourcePageType().Name;
        if (pageType == xaml_typename<PlaylistPage>().Name)
        {
            App::Navigate<MainPage>();
        }

        if (pageType == xaml_typename<MainPage>().Name)
        {
            App::Navigate<PlaylistPage>();
        }
    }

    void MainWindow::Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        UpdateUI();
    }

    void MainWindow::UpdateMediaName()
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
    }

    void MainWindow::UpdateUI()
    {
        UpdateMediaName();

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

    void MainWindow::UpdateTimeline()
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
