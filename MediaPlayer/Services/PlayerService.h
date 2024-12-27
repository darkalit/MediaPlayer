#pragma once

struct winrt::Windows::Foundation::Uri;
struct IMFMediaSource;
struct IMFMediaSession;

class PlayerService
{
public:
    struct MediaMetadata
    {
        long long duration = 0; // duration in milliseconds

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

    enum class State
    {
        CLOSED = 0,
        READY,
        STOPPED,
        PLAYING,
        PAUSED,
    };

    PlayerService();
    ~PlayerService();

    void SetSource(const winrt::Windows::Foundation::Uri& path);
    bool HasSource();

    State GetState();

    void Start();
    void Stop();
    void Pause();
    void Play();
    void Seek(long long time);

    long long GetPosition();
    long long GetRemaining();
    static std::wstring DurationToWString(long long duration);

    std::optional<MediaMetadata> GetMetadata();

private:
    template <typename T>
    void SafeRelease(T*& object);

    std::optional<MediaMetadata> GetMetadataInternal();

    long long m_Position = 0;
    State m_State = State::CLOSED;

    std::optional<MediaMetadata> m_Metadata;
    IMFMediaSource* m_Source = nullptr;
    IMFMediaSession* m_MediaSession = nullptr;
};

template <typename T>
void PlayerService::SafeRelease(T*& object)
{
    if (object)
    {
        object->Release();
        object = nullptr;
    }
}
