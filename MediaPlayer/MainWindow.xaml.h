#pragma once

#include "MainWindow.g.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        void OnLoad(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile> OpenFilePickerAsync();

        fire_and_forget MenuItem_OpenFile_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void MenuItem_Exit_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void MenuItem_PlaybackSpeed_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Button_PlayPause_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Slider_Volume_PointerMoved(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Button_Prev_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Button_Next_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Button_Playlist_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        template <typename T>
        void Navigate();

        template <typename T>
        void Navigate(IInspectable const& parameter);

    private:
        void UpdateMediaName();
        void UpdateUI();
        void UpdateTimeline();

        Microsoft::UI::Dispatching::DispatcherQueueTimer m_TimelineDispatcherTimer;
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
