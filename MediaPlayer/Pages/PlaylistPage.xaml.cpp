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
        m_Playlist(App::GetPlayerService()->GetPlaylist())
    {        
    }

    IVector<MediaMetadata> PlaylistPage::Playlist()
    {
        return m_Playlist;
    }
}
