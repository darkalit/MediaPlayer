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

    fire_and_forget MainPage::OpenFileButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto file = co_await OpenFilePickerAsync();
        if (!file) {
            MetadataTextBlock().Text(L"");
            co_return;
        }

        m_PlayerService.SetSource(Uri(file.Path()));
        auto metadata = m_PlayerService.GetMetadata();
        MetadataTextBlock().Text(metadata.albumTitle.value_or(L"empty"));
    }
}
