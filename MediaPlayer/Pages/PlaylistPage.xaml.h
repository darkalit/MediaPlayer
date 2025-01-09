#pragma once

#include "PlaylistPage.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct PlaylistPage : PlaylistPageT<PlaylistPage>
    {
        PlaylistPage();
        Windows::Foundation::Collections::IVector<MediaMetadata> Playlist();

    private:
        Windows::Foundation::Collections::IVector<MediaMetadata> m_Playlist;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct PlaylistPage : PlaylistPageT<PlaylistPage, implementation::PlaylistPage>
    {
    };
}
