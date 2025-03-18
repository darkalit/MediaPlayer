#pragma once

class MediaUrlGetter
{
public:
    // video first, audio second
    static std::vector<winrt::hstring> GetYoutubeStreamUrls(winrt::hstring const& url);
};

