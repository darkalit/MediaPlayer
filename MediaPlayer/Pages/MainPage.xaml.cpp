#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "App.xaml.h"
#include "winrt/Windows.Storage.Pickers.h"
#include "winrt/Windows.Foundation.h"
#include "ShObjIdl.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;
using namespace Windows::Media;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    MainPage::MainPage()
        :
        m_TimelineDispatcherTimer(Dispatching::DispatcherQueue::GetForCurrentThread().CreateTimer())
    {
        m_TimelineDispatcherTimer.Interval(std::chrono::milliseconds(100));
        m_TimelineDispatcherTimer.Tick([this](auto const&, auto const&) {
            this->UpdateUI();
        });
        m_TimelineDispatcherTimer.Start();
    }

    void MainPage::OnLoad(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
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

        m_PlayerService.Init(SwapChainPanel_Video());

        //m_DeviceResources.SetSwapChainPanel(SwapChainPanel_Video(), App::GetMainWindow());
    }

    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile> MainPage::OpenFilePickerAsync()
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

        co_return co_await filePicker.PickSingleFileAsync();
    }

    fire_and_forget MainPage::MenuItem_OpenFile_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (m_PlayerService.HasSource())
        {
            m_PlayerService.Stop();
        }

        auto file = co_await OpenFilePickerAsync();
        if (!file) {
            co_return;
        }

        m_PlayerService.SetSource(Uri(file.Path()));
        auto metadata = m_PlayerService.GetMetadata();

        std::wstring title = L"";
        std::wstring authorAlbum = L"";

        if (metadata && metadata->title)
        {
            title = *metadata->title;
        }
        else
        {
            title = file.Name();
        }

        if (metadata && metadata->author)
        {
            authorAlbum += *metadata->author;
        }

        if (metadata && metadata->albumTitle)
        {
            authorAlbum += (metadata->author ? L" - " : L"") + *metadata->albumTitle;
        }

        TextBlock_Title().Text(title);
        TextBlock_AuthorAlbum().Text(authorAlbum);

        if (metadata)
        {
            Slider_Timeline().Maximum(metadata->duration / 1000.0);
        }   

        UpdateUI();
    }

    void MainPage::MenuItem_Exit_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        m_PlayerService.Stop();
        Application::Current().Exit();
    }

    void MainPage::Slider_Timeline_PointerMoved(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        UpdateUI();
    }

    void MainPage::SwapChainPanel_Video_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        auto size = SwapChainPanel_Video().ActualSize();
        m_PlayerService.ResizeVideo(size.x, size.y);
    }

    void MainPage::Slider_Timeline_PointerReleased(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        long long time = m_PlayerService.GetMetadata()->duration * (Slider_Timeline().Value() / Slider_Timeline().Maximum());
        m_PlayerService.Start(time);
        UpdateUI();
    }


    void MainPage::Slider_Timeline_PointerPressed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {

        m_PlayerService.Pause();
        UpdateUI();
    }


    void MainPage::Button_PlayPause_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (m_PlayerService.GetState() == PlayerService::State::STOPPED || m_PlayerService.GetState() == PlayerService::State::PAUSED)
        {
            m_PlayerService.Start();
        }
        else if (m_PlayerService.GetState() == PlayerService::State::PLAYING)
        {
            m_PlayerService.Pause();
        }

        UpdateUI();
    }

    void MainPage::UpdateUI()
    {
        UpdateTimeline();

        if (m_PlayerService.GetState() == PlayerService::State::STOPPED || m_PlayerService.GetState() == PlayerService::State::PAUSED)
        {
            Button_PlayPause().Content(box_value(L"Play"));
        }
        else if (m_PlayerService.GetState() == PlayerService::State::PLAYING)
        {
            Button_PlayPause().Content(box_value(L"Pause"));
        }
    }
    void MainPage::UpdateTimeline()
    {
        Slider_Timeline().IsEnabled(m_PlayerService.HasSource());

        if (!m_PlayerService.HasSource()) return;

        if (m_PlayerService.GetState() == PlayerService::State::PLAYING)
        {
            double progress = static_cast<double>(m_PlayerService.GetPosition()) / 1000.0;
            Slider_Timeline().Value(progress);

            if (m_PlayerService.GetRemaining() <= 0)
            {
                m_PlayerService.Stop();
            }
        }
        TextBlock_Position().Text(m_PlayerService.DurationToWString(Slider_Timeline().Value() * 1000.0));
        TextBlock_RemainingTime().Text(m_PlayerService.DurationToWString((Slider_Timeline().Maximum() - Slider_Timeline().Value()) * 1000.0));
    }
}
