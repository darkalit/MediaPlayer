#pragma once

#include "winrt/MediaPlayer.h"

class MediaUrlGetter
{
public:
    static winrt::MediaPlayer::MediaMetadata GetMetadata(winrt::hstring const& url);

    // video first, audio second
    static std::vector<winrt::hstring> GetYoutubeStreamUrls(winrt::hstring const& url);

private:
    static std::string LaunchProcess(winrt::hstring const& cmd);
};

