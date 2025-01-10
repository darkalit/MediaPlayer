#pragma once

#include "winrt/MediaPlayer.h"

#include "Media/MediaEngineWrapper.h"

struct IMFMediaSource;
struct IMFMediaSession;
struct IMFTopology;
struct IMFTopologyNode;
struct IMFPresentationDescriptor;
struct IMFStreamDescriptor;
struct IMFActivate;

namespace winrt::MediaPlayer
{
    class PlayerService
    {
    public:
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
        void Init();

        void SetSwapChainPanel(Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel);

        void AddSource(const hstring& path, const hstring& displayName);
        void SetSource(const hstring& path);
        bool HasSource();

        State GetState();

        void Next();
        void Prev();
        void StartByIndex(int index);
        void Clear();
        int GetCurrentMediaIndex();

        // Start playing from the time in milliseconds or continue playing if time is not specified
        void Start(const std::optional<long long>& time = {});
        void Stop();
        void Pause();
        void SetPlaybackSpeed(double speed);
        void SetVolume(double volume);
        double GetVolume();

        void ResizeVideo(unsigned int width, unsigned int height);

        long long GetPosition(); // in milliseconds
        long long GetRemaining(); // in milliseconds
        static hstring DurationToWString(long long duration);
        MediaMetadata GetMetadata();
        Windows::Foundation::Collections::IVector<MediaMetadata> GetPlaylist();

    private:
        MediaMetadata GetMetadataInternal(const hstring& path);

        void OnLoaded();
        void OnPlaybackEnded();
        void OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr);


        long long m_Position = 0;
        double m_PlaybackSpeed = 1.0;
        State m_State = State::CLOSED;
        Windows::Foundation::Collections::IVector<MediaMetadata> m_MediaPlaylist = single_threaded_observable_vector<MediaMetadata>();
        int m_CurrentMedia = -1;

        com_ptr<IMFDXGIDeviceManager> m_DeviceManager;
        com_ptr<IMFSourceReader> m_SourceReader;
        com_ptr<MediaEngineWrapper> m_MediaEngineWrapper;
        Microsoft::UI::Xaml::Controls::SwapChainPanel m_SwapChainPanel;

        HANDLE m_VideoSurfaceHandle;
    };
}
