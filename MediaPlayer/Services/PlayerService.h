#pragma once

struct winrt::Windows::Foundation::Uri;
struct IMFMediaSource;

class PlayerService
{
public:
    struct MediaMetadata
    {

    };

    PlayerService();
    ~PlayerService();

    void SetSource(winrt::Windows::Foundation::Uri path);

    // TODO: output struct
    std::wstring GetMetadata();

private:
    IMFMediaSource* m_Source = nullptr;
};

