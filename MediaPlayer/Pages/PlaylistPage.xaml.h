#pragma once

#include "PlaylistPage.g.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct PlaylistPage : PlaylistPageT<PlaylistPage>
    {
        PlaylistPage();
        void OnLoad(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::SizeChangedEventArgs const&);
        void ItemsView_Playlist_ItemInvoked(Microsoft::UI::Xaml::Controls::ItemsView const& sender, Microsoft::UI::Xaml::Controls::ItemsViewItemInvokedEventArgs const& args);
        void Button_ClearPlaylist_Click(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        Windows::Foundation::Collections::IVector<MediaMetadata> Playlist();

    private:
        std::shared_ptr<PlayerService> m_PlayerService;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct PlaylistPage : PlaylistPageT<PlaylistPage, implementation::PlaylistPage>
    {
    };
}
