#include "pch.h"
#include "PlaylistPage.xaml.h"
#if __has_include("PlaylistPage.g.cpp")
#include "PlaylistPage.g.cpp"
#endif

#include "App.xaml.h"
#include "windows.foundation.collections.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Foundation::Collections;

namespace winrt::MediaPlayer::implementation
{
    PlaylistPage::PlaylistPage()
        :
        m_PlayerService(App::GetPlayerService())
    {
    }

    void PlaylistPage::OnLoad(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService.SwapChainPanel(SwapChainPanel_Video());
    }

    void PlaylistPage::SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, SizeChangedEventArgs const&)
    {
        auto size = SwapChainPanel_Video().ActualSize();
        m_PlayerService.ResizeVideo(size.x, size.y);
    }

    void PlaylistPage::ItemsView_Playlist_ItemInvoked(Controls::ItemsView const& sender, Controls::ItemsViewItemInvokedEventArgs const&)
    {
        auto index = sender.CurrentItemIndex();
        m_PlayerService.StartByIndex(index);
    }

    void PlaylistPage::Button_ClearPlaylist_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService.Clear();
    }

    void PlaylistPage::Button_DeleteItem_Click(Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.as<Controls::Button>();
        auto metadata = button.DataContext().try_as<MediaMetadata>();

        if (metadata)
        {
            unsigned int index;
            m_PlayerService.Playlist().IndexOf(*metadata, index);
            m_PlayerService.DeleteByIndex(index);
        }
    }

    IPlayerService PlaylistPage::PlayerService()
    {
        return m_PlayerService;
    }

    IVector<MediaMetadata> PlaylistPage::Playlist()
    {
        return m_PlayerService.Playlist();
    }
}
