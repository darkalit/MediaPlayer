#pragma once

#include "PlayerService.g.h"
#include "DirectX/TexturePlaneRenderer.h"

#include "winrt/MediaPlayer.h"
#include "Media/FfmpegDecoder.h"
#include "Media/MediaEngineWrapper.h"

struct IMFMediaSource;
struct IMFMediaSession;
struct IMFTopology;
struct IMFTopologyNode;
struct IMFPresentationDescriptor;
struct IMFStreamDescriptor;
struct IMFActivate;
struct IDXGISwapChain1;
struct ID3D11Texture2D;

namespace winrt::MediaPlayer::implementation
{
    struct PlayerService : PlayerServiceT<PlayerService>
    {
        PlayerService();
        ~PlayerService() override;
        void Init();

        void AddSource(hstring const& path, hstring const& displayName);
        void SetSource(hstring const& path);
        bool HasSource();
        int32_t GetMediaIndexById(guid const& id);

        void Next();
        void Prev();
        void StartByIndex(int32_t index);
        void DeleteByIndex(int32_t index);
        void Clear();
        void Start();
        void Start(uint64_t timePos);
        void Stop();
        void Pause();

        void ResizeVideo(float width, float height);

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
        Microsoft::UI::Dispatching::DispatcherQueue UIDispatcher();
        void UIDispatcher(Microsoft::UI::Dispatching::DispatcherQueue const& value);

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

        Windows::Foundation::Size m_LastFrameSize;
        std::thread m_VideoThread;
        std::shared_ptr<DeviceResources> m_DeviceResources;
        std::shared_ptr<TexturePlaneRenderer> m_TexturePlaneRenderer;
        com_ptr<ID3D11ShaderResourceView> m_ShaderResourceView;
        com_ptr<IMFDXGIDeviceManager> m_DeviceManager;
        com_ptr<IMFSourceReader> m_SourceReader;
        com_ptr<MediaEngineWrapper> m_MediaEngineWrapper;
        Microsoft::UI::Xaml::Controls::SwapChainPanel m_SwapChainPanel;
        Microsoft::UI::Dispatching::DispatcherQueue m_UIDispatcherQueue = nullptr;

        FfmpegDecoder m_FfmpegDecoder;

        HANDLE m_VideoSurfaceHandle;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct PlayerService : PlayerServiceT<PlayerService, implementation::PlayerService>
    {
    };
}
