#pragma once

#include "MainPage.g.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        void OnLoad(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnUnload(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        void SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Button_PlayPause_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Slider_Volume_PointerMoved(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Button_Prev_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Button_Next_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Button_Playlist_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        void UpdateUI();
        void UpdateTimeline();

        Microsoft::UI::Dispatching::DispatcherQueueTimer m_TimelineDispatcherTimer;
        std::shared_ptr<PlayerService> m_PlayerService;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
