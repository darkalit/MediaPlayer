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
using namespace Microsoft::UI::Xaml;
using namespace Windows::Media;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Foundation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::MediaPlayer::implementation
{
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

        UpdateTimeline();
    }

    void MainPage::MenuItem_Exit_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        Application::Current().Exit();
    }

    void MainPage::Slider_Timeline_PointerMoved(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        unsigned int time = m_PlayerService.GetMetadata()->duration * Slider_Timeline().Value() / Slider_Timeline().Maximum();
        m_PlayerService.Seek(time);
        UpdateTimeline();
    }

    void MainPage::UpdateTimeline()
    {
        Slider_Timeline().IsEnabled(m_PlayerService.HasSource());
        TextBlock_Position().Text(m_PlayerService.DurationToWString(m_PlayerService.GetPosition()));
        TextBlock_RemainingTime().Text(m_PlayerService.DurationToWString(m_PlayerService.GetRemaining()));
    }
}
