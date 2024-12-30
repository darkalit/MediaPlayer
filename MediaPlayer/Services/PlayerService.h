#pragma once

#include "mfobjects.h"

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
    class StateHandler : public IMFAsyncCallback
    {
    public:
        StateHandler(PlayerService* playerServiceRef);

        STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
        STDMETHODIMP_(ULONG) AddRef() override;
        STDMETHODIMP_(ULONG) Release() override;
        STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue) override;
        STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult) override;

    private:
        long m_RefCount;
        PlayerService* m_PlayerServiceRef;
    };

    static void AddSourceNode(
        winrt::com_ptr<IMFTopology>& topology,
        winrt::com_ptr<IMFMediaSource>& source,
        winrt::com_ptr<IMFPresentationDescriptor>& presentationDescriptor,
        winrt::com_ptr<IMFStreamDescriptor>& streamDescriptor,
        winrt::com_ptr<IMFTopologyNode>& node);
    static void AddOutputNode(
        winrt::com_ptr<IMFTopology>& topology,
        winrt::com_ptr<IMFActivate>& activate,
        winrt::com_ptr<IMFTopologyNode>& node);
    std::optional<MediaMetadata> GetMetadataInternal();

    long long m_Position = 0;
    State m_State = State::CLOSED;

    std::optional<MediaMetadata> m_Metadata;
    StateHandler m_StateHandler;
    winrt::com_ptr<IMFMediaSource> m_Source;
    winrt::com_ptr<IMFMediaSession> m_MediaSession;
};
