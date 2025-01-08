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
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    MainWindow::MainWindow()
        :
        m_PlayerService(App::GetPlayerService())
    {
        this->Title(L"MediaPlayer");
    }

    IAsyncOperation<StorageFile> MainWindow::OpenFilePickerAsync()
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

    fire_and_forget MainWindow::MenuItem_OpenFile_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        auto file = co_await OpenFilePickerAsync();
        if (!file) {
            co_return;
        }

        m_PlayerService->AddSource(Uri(file.Path()), file.DisplayName());
    }

    void MainWindow::MenuItem_Exit_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        m_PlayerService->Stop();
        Application::Current().Exit();
    }

    void MainWindow::MenuItem_PlaybackSpeed_Click(winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
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
}
