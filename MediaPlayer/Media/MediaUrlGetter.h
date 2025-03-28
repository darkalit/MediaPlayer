#pragma once

#include "winrt/MediaPlayer.h"

enum class WebResourceType
{
    Youtube,
    OneDrive,

    Unknown,
};

class MediaUrlGetter
{
public:
    static WebResourceType GetType(winrt::hstring const& url);
    static winrt::MediaPlayer::MediaMetadata GetMetadata(winrt::hstring const& url);

    // video first, audio second
    static std::vector<winrt::hstring> GetYoutubeStreamUrls(winrt::hstring const& url);
    static winrt::hstring GetOneDriveStreamUrl(winrt::hstring const& url);

private:
    static std::string LaunchProcess(winrt::hstring const& cmd);
};

