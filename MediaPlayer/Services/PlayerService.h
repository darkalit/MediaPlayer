#pragma once

struct winrt::Windows::Foundation::Uri;
struct IMFMediaSource;

class PlayerService
{
public:
    struct MediaMetadata
    {
        unsigned long long duration = 0; // duration in milliseconds

        std::optional<unsigned int> audioChannelCount;
        std::optional<unsigned int> audioBitrate; // bits per second
        std::optional<unsigned int> audioSampleRate;
        std::optional<unsigned int> audioSampleSize;
        std::optional<unsigned int> audioStreamId;

        std::optional<unsigned int> videoBitrate; // bits per second
        std::optional<unsigned int> videoWidth;
        std::optional<unsigned int> videoHeight;
        std::optional<unsigned int> videoFrameRate;
        std::optional<unsigned int> videoStreamId;

        std::optional<std::wstring> author;
        std::optional<std::wstring> title;
        std::optional<std::wstring> albumTitle;

        std::optional<bool> audioIsVariableBitrate;
        std::optional<bool> videoIsStereo;
        
        // { PKEY_Audio_Format, L"Audio format" },
    };

    PlayerService();
    ~PlayerService();

    void SetSource(winrt::Windows::Foundation::Uri path);

    MediaMetadata GetMetadata();

private:
    IMFMediaSource* m_Source = nullptr;
};

