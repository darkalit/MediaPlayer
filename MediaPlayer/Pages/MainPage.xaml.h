#pragma once

#include "MainPage.g.h"
#include "ViewModels/MainPageViewModel.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        void OnLoad(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnUnload(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

        MediaPlayer::MainPageViewModel ViewModel();

    private:
        MediaPlayer::MainPageViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
