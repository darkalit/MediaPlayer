#pragma once

#include "Media/MediaEngineWrapper.h"

struct winrt::Windows::Foundation::Uri;
struct IMFMediaSource;
struct IMFMediaSession;
struct IMFTopology;
struct IMFTopologyNode;
struct IMFPresentationDescriptor;
struct IMFStreamDescriptor;
struct IMFActivate;

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

        std::optional<winrt::hstring> author;
        std::optional<winrt::hstring> title;
        std::optional<winrt::hstring> albumTitle;

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
    void Init(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel, const winrt::Windows::Foundation::Uri& path);

    void SetSource(const winrt::Windows::Foundation::Uri& path, HWND hwnd = nullptr);
    bool HasSource();

    State GetState();

    void Start(const std::optional<long long>& time = {});
    void Stop();
    void Pause();
    void Play();

    long long GetPosition();
    long long GetRemaining();
    static winrt::hstring DurationToWString(long long duration);

    std::optional<MediaMetadata> GetMetadata();

private:
    void OnLoaded();
    void OnPlaybackEnded();
    void OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr);
    std::optional<MediaMetadata> GetMetadataInternal();

    long long m_Position = 0;
    State m_State = State::CLOSED;

    //std::optional<MediaMetadata> m_Metadata;
    //StateHandler m_StateHandler;

    winrt::com_ptr<IMFDXGIDeviceManager> m_DeviceManager;
    winrt::com_ptr<IMFSourceReader> m_SourceReader;
    winrt::com_ptr<MediaEngineWrapper> m_MediaEngineWrapper;
    winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel m_SwapChainPanel;

    HANDLE m_VideoSurfaceHandle;
};
