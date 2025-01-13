#pragma once
#include "PlayerService.g.h"

#include "winrt/MediaPlayer.h"
#include "Media/MediaEngineWrapper.h"

struct IMFMediaSource;
struct IMFMediaSession;
struct IMFTopology;
struct IMFTopologyNode;
struct IMFPresentationDescriptor;
struct IMFStreamDescriptor;
struct IMFActivate;

namespace winrt::MediaPlayer::implementation
{
    struct PlayerService : PlayerServiceT<PlayerService>
    {
        PlayerService();
        ~PlayerService() override;
        void Init();

        static hstring DurationToString(uint64_t duration);

        void AddSource(hstring const& path, hstring const& displayName);
        void SetSource(hstring const& path);
        bool HasSource();

        void Next();
        void Prev();
        void StartByIndex(int32_t index);
        void DeleteByIndex(int32_t index);
        void Clear();
        void Start();
        void Start(uint64_t timePos);
        void Stop();
        void Pause();

        void ResizeVideo(uint32_t width, uint32_t height);

        uint64_t Position();
        void Position(uint64_t value);
        uint64_t RemainingTime();
        double PlaybackSpeed();
        void PlaybackSpeed(double value);
        int32_t CurrentMediaIndex();
        void CurrentMediaIndex(int32_t value);
        double Volume();
        void Volume(double value);
        PlayerServiceState State();
        void State(PlayerServiceState value);
        MediaMetadata Metadata();
        void Metadata(MediaMetadata const& value);
        Windows::Foundation::Collections::IVector<MediaMetadata> Playlist();
        Microsoft::UI::Xaml::Controls::SwapChainPanel SwapChainPanel();
        void SwapChainPanel(Microsoft::UI::Xaml::Controls::SwapChainPanel const& value);
        event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(event_token const& token) noexcept;

    private:
        MediaMetadata GetMetadataInternal(hstring const& path);

        void OnLoaded();
        void OnPlaybackEnded();
        void OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr);


        uint64_t m_Position = 0;
        double m_PlaybackSpeed = 1.0;
        PlayerServiceState m_State = PlayerServiceState::CLOSED;
        Windows::Foundation::Collections::IVector<MediaMetadata> m_Playlist = single_threaded_observable_vector<MediaMetadata>();
        int32_t m_CurrentMediaIndex = -1;

        com_ptr<IMFDXGIDeviceManager> m_DeviceManager;
        com_ptr<IMFSourceReader> m_SourceReader;
        com_ptr<MediaEngineWrapper> m_MediaEngineWrapper;
        Microsoft::UI::Xaml::Controls::SwapChainPanel m_SwapChainPanel;

        HANDLE m_VideoSurfaceHandle;

        event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_PropertyChanged;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct PlayerService : PlayerServiceT<PlayerService, implementation::PlayerService>
    {
    };
}
