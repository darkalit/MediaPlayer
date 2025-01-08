#pragma once

#include "MainWindow.g.h"

class PlayerService;

namespace winrt::MediaPlayer::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile> OpenFilePickerAsync();

        fire_and_forget MenuItem_OpenFile_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void MenuItem_Exit_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void MenuItem_PlaybackSpeed_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        template <typename T>
        void Navigate();

        template <typename T>
        void Navigate(IInspectable const& parameter);

    private:
        std::shared_ptr<PlayerService> m_PlayerService;
    };

    template <typename T>
    void MainWindow::Navigate()
    {
        PageFrame().Navigate(xaml_typename<T>());
    }

    template <typename T>
    void MainWindow::Navigate(IInspectable const& parameter)
    {
        PageFrame().Navigate(xaml_typename<T>(), parameter);
    }
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
