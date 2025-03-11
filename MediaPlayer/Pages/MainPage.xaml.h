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
        void Up_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender, Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args);
        void Down_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender, Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args);
        void Left_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender, Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args);
        void Right_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender, Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args);
        void SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerEntered(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void Slider_Timeline_PointerExited(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

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
