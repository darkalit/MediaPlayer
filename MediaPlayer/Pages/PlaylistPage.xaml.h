#pragma once

#include "PlaylistPage.g.h"
#include "ViewModels/PlaylistViewModel.h"

namespace winrt::MediaPlayer::implementation
{
    struct PlaylistPage : PlaylistPageT<PlaylistPage>
    {
        PlaylistPage();
        void OnLoad(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void ItemsView_Playlist_ItemInvoked(Microsoft::UI::Xaml::Controls::ItemsView const& sender, Microsoft::UI::Xaml::Controls::ItemsViewItemInvokedEventArgs const& args);
        void Button_DeleteItem_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

        MediaPlayer::PlaylistViewModel ViewModel();

    private:
        MediaPlayer::PlaylistViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct PlaylistPage : PlaylistPageT<PlaylistPage, implementation::PlaylistPage>
    {
    };
}
