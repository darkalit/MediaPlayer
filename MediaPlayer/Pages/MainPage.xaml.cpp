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
        m_TimelineDispatcherTimer.Interval(std::chrono::milliseconds(200));
        m_TimelineDispatcherTimer.Tick([this](auto const&, auto const&) {
            this->UpdateTimeline();
        });
        m_TimelineDispatcherTimer.Start();
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

        if (metadata && metadata->title)
        {
            title += *metadata->title;
        }
        else
        {
            title += file.Name();
        }

        if (metadata && metadata->author)
        {
            title += L" - " + *metadata->author;
        }

        TextBlock_Title().Text(title);

        if (metadata)
        {
            Slider_Timeline().Maximum(metadata->duration / 1000.0);
        }

        //Slider_Timeline().PointerPressed({ this, &MainPage::Slider_Timeline_PointerPressed });

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

        UpdateUI();
    }

    void MainPage::MenuItem_Exit_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        Application::Current().Exit();
    }

    void MainPage::Slider_Timeline_PointerMoved(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        UpdateUI();
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
            m_PlayerService.Play();
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
        }
        TextBlock_Position().Text(m_PlayerService.DurationToWString(Slider_Timeline().Value() * 1000.0));
        TextBlock_RemainingTime().Text(m_PlayerService.DurationToWString((Slider_Timeline().Maximum() - Slider_Timeline().Value()) * 1000.0));
    }
}
