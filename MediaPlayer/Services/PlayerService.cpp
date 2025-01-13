#include "pch.h"
#include "PlayerService.h"
#include "PlayerService.g.cpp"

#include "mfapi.h"
#include "mfplay.h"
#include "mfobjects.h"
#include "mfidl.h"
#include "propvarutil.h"
#include "propkeydef.h"
#include "propkey.h"
#include "Shlwapi.h"

#include "string"
#include "iostream"

#include "Audioclient.h"
#include "microsoft.ui.xaml.media.dxinterop.h"
#include "d3d11.h"

using namespace winrt::Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    PlayerService::PlayerService()
    {
        try
        {
            check_hresult(MFStartup(MF_VERSION));

            //Init();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::PlayerService: Failed to init Media Foundation;\n" << e.message() << '\n';
        }
    }

    PlayerService::~PlayerService()
    {
        try
        {
            check_hresult(MFShutdown());
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::~PlayerService: Failed to shutdown Media Foundation;\n" << e.message() << '\n';
        }
    }

    hstring PlayerService::DurationToString(uint64_t duration)
    {
        if (duration == 0)
        {
            return L"00:00:00";
        }

        unsigned int hours = duration / (1000 * 60 * 60);
        unsigned int minutes = (duration / (1000 * 60)) % 60;
        unsigned int seconds = (duration / 1000) % 60;

        return (hours < 10 ? L"0" : L"") + to_hstring(hours) + L":" + (minutes < 10 ? L"0" : L"") + to_hstring(minutes) + L":" + (seconds < 10 ? L"0" : L"") + to_hstring(seconds);
    }

    void PlayerService::Init()
    {
        m_DeviceManager = nullptr;
        UINT resetToken = 0;
        check_hresult(MFLockDXGIDeviceManager(&resetToken, m_DeviceManager.put()));

        com_ptr<ID3D11Device> d3d11Device;
        UINT creationgFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT
            | D3D11_CREATE_DEVICE_BGRA_SUPPORT
            | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
        constexpr D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        check_hresult(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            creationgFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            d3d11Device.put(),
            nullptr,
            nullptr
        ));

        com_ptr<ID3D10Multithread> multithreadedDevice;
        d3d11Device.as(multithreadedDevice);
        multithreadedDevice->SetMultithreadProtected(true);

        check_hresult(m_DeviceManager->ResetDevice(d3d11Device.get(), resetToken));

        auto onLoadedCB = std::bind(&PlayerService::OnLoaded, this);
        auto onPlaybackEndedCB = std::bind(&PlayerService::OnPlaybackEnded, this);
        auto onErrorCB = std::bind(&PlayerService::OnError, this, std::placeholders::_1, std::placeholders::_2);

        m_MediaEngineWrapper = make_self<MediaEngineWrapper>(
            m_DeviceManager.get(),
            onLoadedCB,
            onPlaybackEndedCB,
            onErrorCB,
            0,
            0);

        State(PlayerServiceState::READY);
    }

    void PlayerService::AddSource(hstring const& path, hstring const& displayName)
    {
        auto res = GetMetadataInternal(path);

        if (res.Title.empty())
        {
            res.Title = displayName;
        }

        m_Playlist.Append(res);

        if (CurrentMediaIndex() == -1)
        {
            CurrentMediaIndex(0);
        }

        if (!HasSource())
        {
            SetSource(path);
        }
    }

    void PlayerService::SetSource(hstring const& path)
    {
        check_hresult(MFCreateSourceReaderFromURL(path.c_str(), nullptr, m_SourceReader.put()));
        m_MediaEngineWrapper->SetSource(m_SourceReader.get());
        if (SwapChainPanel()) {
            SwapChainPanel().DispatcherQueue().TryEnqueue([&]()
                {
                    auto size = SwapChainPanel().ActualSize();
                    m_MediaEngineWrapper->WindowUpdate(size.x, size.y);
                });
        }
        Position(0);
        State(PlayerServiceState::STOPPED);
    }

    bool PlayerService::HasSource()
    {
        return !!m_SourceReader && m_MediaEngineWrapper && m_Playlist.Size() > 0 && CurrentMediaIndex() > -1;
    }

    void PlayerService::Next()
    {
        if (!HasSource()) return;

        Stop();
        CurrentMediaIndex(CurrentMediaIndex() + 1);
        CurrentMediaIndex(CurrentMediaIndex() % m_Playlist.Size());
        SetSource(Metadata().Path);
        Start();
    }

    void PlayerService::Prev()
    {
        if (!HasSource()) return;

        Stop();
        if (CurrentMediaIndex() == 0)
        {
            CurrentMediaIndex(m_Playlist.Size());
        }
        CurrentMediaIndex(CurrentMediaIndex() - 1);
        SetSource(Metadata().Path);
        Start();
    }

    void PlayerService::StartByIndex(int32_t index)
    {
        if (!HasSource()) return;

        CurrentMediaIndex(max(0, min(m_Playlist.Size() - 1, index)));
        SetSource(Metadata().Path);
        Start();
    }

    void PlayerService::DeleteByIndex(int32_t index)
    {
        if (!HasSource()) return;
        if (index >= m_Playlist.Size() || index < 0) return;

        m_Playlist.RemoveAt(index);

        if (m_Playlist.Size() == 0)
        {
            Stop();
            return;
        }

        if (index < CurrentMediaIndex())
        {
            CurrentMediaIndex(CurrentMediaIndex() - 1);
            return;
        }

        index %= m_Playlist.Size();
        int newCurrentMedia = CurrentMediaIndex() % m_Playlist.Size();

        if (newCurrentMedia == index || newCurrentMedia != CurrentMediaIndex())
        {
            CurrentMediaIndex(newCurrentMedia);
            Stop();
            SetSource(Metadata().Path);
            Start();
        }
    }

    void PlayerService::Clear()
    {
        if (!HasSource()) return;

        Stop();
        m_Playlist.Clear();
        CurrentMediaIndex(-1);
    }

    void PlayerService::Start()
    {
        if (!HasSource()) return;

        try
        {
            if (CurrentMediaIndex() > -1 && Position() >= Metadata().Duration)
            {
                Stop();
                return;
            }

            double position = static_cast<double>(Position()) / 1000.0;
            m_MediaEngineWrapper->Start(position);
            PlaybackSpeed(m_PlaybackSpeed);
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Start: Failed to start media;\n" << e.message() << '\n';
        }

        State(PlayerServiceState::PLAYING);
    }

    void PlayerService::Start(uint64_t timePos)
    {
        if (!HasSource()) return;

        try
        {
            Position(timePos);

            if (CurrentMediaIndex() > -1 && Position() >= Metadata().Duration)
            {
                Stop();
                return;
            }

            double position = static_cast<double>(Position()) / 1000.0;
            m_MediaEngineWrapper->Start(position);
            PlaybackSpeed(m_PlaybackSpeed);
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Start: Failed to start media;\n" << e.message() << '\n';
        }

        State(PlayerServiceState::PLAYING);
    }

    void PlayerService::Stop()
    {
        State(PlayerServiceState::STOPPED);

        try
        {
            m_Position = 0;
            m_MediaEngineWrapper->Stop();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Stop: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::Pause()
    {
       State(PlayerServiceState::PAUSED);

        try
        {
            m_MediaEngineWrapper->Pause();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Pause: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::ResizeVideo(uint32_t width, uint32_t height)
    {
        if (!m_MediaEngineWrapper) return;
        m_MediaEngineWrapper->WindowUpdate(width, height);
    }

    uint64_t PlayerService::Position()
    {
        m_Position = m_MediaEngineWrapper->GetCurrentTime() * 1000;
        return m_Position;
    }

    void PlayerService::Position(uint64_t value)
    {

    }

    uint64_t PlayerService::RemainingTime()
    {
        auto metadata = Metadata();
        return metadata.Duration ? metadata.Duration - m_Position : 0;
    }

    double PlayerService::PlaybackSpeed()
    {
        return m_PlaybackSpeed;
    }

    void PlayerService::PlaybackSpeed(double value)
    {
        if (m_PlaybackSpeed != value)
        {
            m_PlaybackSpeed = value;
            m_MediaEngineWrapper->SetPlaybackSpeed(value);
            m_PropertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"PlaybackSpeed" });
        }
    }

    int32_t PlayerService::CurrentMediaIndex()
    {
        return m_CurrentMediaIndex;
    }

    void PlayerService::CurrentMediaIndex(int32_t value)
    {
        if (m_CurrentMediaIndex != value)
        {
            m_CurrentMediaIndex = value;
            m_PropertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"CurrentMediaIndex" });
        }
    }

    double PlayerService::Volume()
    {
        if (!HasSource()) return 0;

        return m_MediaEngineWrapper->GetVolume();
    }

    void PlayerService::Volume(double value)
    {
        if (m_MediaEngineWrapper->GetVolume() != value)
        {
            m_MediaEngineWrapper->SetVolume(value);
            m_PropertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"Volume" });
        }        
    }

    PlayerServiceState PlayerService::State()
    {
        return m_State;
    }

    void PlayerService::State(PlayerServiceState value)
    {
        if (m_State != value)
        {
            m_State = value;
            m_PropertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"State" });
        }
    }

    MediaMetadata PlayerService::Metadata()
    {
        if (!HasSource()) return {};

        return m_Playlist.GetAt(CurrentMediaIndex());
    }

    void PlayerService::Metadata(MediaMetadata const& value)
    {
        if (m_Playlist.GetAt(CurrentMediaIndex()) != value)
        {
            m_Playlist.GetAt(CurrentMediaIndex()) = value;
            m_PropertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"Metadata" });
        }
    }

    Collections::IVector<MediaMetadata> PlayerService::Playlist()
    {
        return m_Playlist;
    }

    Microsoft::UI::Xaml::Controls::SwapChainPanel PlayerService::SwapChainPanel()
    {
        return m_SwapChainPanel;
    }

    void PlayerService::SwapChainPanel(Microsoft::UI::Xaml::Controls::SwapChainPanel const& value)
    {
        if (m_SwapChainPanel != value)
        {
            m_SwapChainPanel = value;
            m_VideoSurfaceHandle = m_MediaEngineWrapper ? m_MediaEngineWrapper->GetSurfaceHandle() : nullptr;

            if (m_VideoSurfaceHandle && m_SwapChainPanel)
            {
                m_SwapChainPanel.DispatcherQueue().TryEnqueue([&]()
                    {
                        com_ptr<ISwapChainPanelNative2> panelNative;
                        m_SwapChainPanel.as(panelNative);
                        check_hresult(panelNative->SetSwapChainHandle(m_VideoSurfaceHandle));
                        auto size = m_SwapChainPanel.ActualSize();
                        m_MediaEngineWrapper->WindowUpdate(size.x, size.y);
                    });
            }
            m_PropertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"SwapChainPanel" });
        }
    }

    event_token PlayerService::PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_PropertyChanged.add(handler);
    }

    void PlayerService::PropertyChanged(event_token const& token) noexcept
    {
        m_PropertyChanged.remove(token);
    }

    MediaMetadata PlayerService::GetMetadataInternal(
        const hstring& path)
    {
        com_ptr<IPropertyStore> props;
        com_ptr<IMFSourceResolver> sourceResolver;
        com_ptr<::IUnknown> source;
        MediaMetadata metadata;

        metadata.Id = GuidHelper::CreateNewGuid();
        metadata.Path = path;

        try
        {
            DWORD cProps;
            MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;

            check_hresult(MFCreateSourceResolver(sourceResolver.put()));
            check_hresult(sourceResolver->CreateObjectFromURL(
                path.c_str(),
                MF_RESOLUTION_MEDIASOURCE,
                nullptr,
                &objectType,
                source.put()
            ));

            check_hresult(MFGetService(source.get(), MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(props.put())));

            check_hresult(props->GetCount(&cProps));

            for (DWORD i = 0; i < cProps; i++)
            {
                PROPERTYKEY key;
                PROPVARIANT pv;

                check_hresult(props->GetAt(i, &key));
                check_hresult(props->GetValue(key, &pv));

                if (key == PKEY_Media_Duration)
                {
                    ULONGLONG duration;
                    check_hresult(PropVariantToUInt64(pv, &duration));
                    metadata.Duration = duration / 10000;
                }
                else if (key == PKEY_Audio_ChannelCount)
                {
                    metadata.AudioChannelCount = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioChannelCount));
                }
                else if (key == PKEY_Audio_EncodingBitrate)
                {
                    metadata.AudioBitrate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioBitrate));
                }
                else if (key == PKEY_Audio_SampleRate)
                {
                    metadata.AudioSampleRate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioSampleRate));
                }
                else if (key == PKEY_Audio_SampleSize)
                {
                    metadata.AudioSampleSize = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioSampleSize));
                }
                else if (key == PKEY_Audio_StreamNumber)
                {
                    metadata.AudioStreamId = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioStreamId));
                }
                else if (key == PKEY_Video_EncodingBitrate)
                {
                    metadata.VideoBitrate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoBitrate));
                }
                else if (key == PKEY_Video_FrameWidth)
                {
                    metadata.VideoWidth = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoWidth));
                }
                else if (key == PKEY_Video_FrameHeight)
                {
                    metadata.VideoHeight = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoHeight));
                }
                else if (key == PKEY_Video_FrameRate)
                {
                    metadata.VideoFrameRate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoFrameRate));
                    metadata.VideoFrameRate /= 1000;
                }
                else if (key == PKEY_Video_StreamNumber)
                {
                    metadata.VideoStreamId = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoStreamId));
                }
                else if (key == PKEY_Author)
                {
                    wchar_t* buffer;
                    check_hresult(PropVariantToStringAlloc(pv, &buffer));
                    metadata.Author = hstring(buffer);
                    CoTaskMemFree(buffer);
                }
                else if (key == PKEY_Title)
                {
                    wchar_t* buffer;
                    check_hresult(PropVariantToStringAlloc(pv, &buffer));
                    metadata.Title = hstring(buffer);
                    CoTaskMemFree(buffer);
                }
                else if (key == PKEY_Music_AlbumTitle)
                {
                    wchar_t* buffer;
                    check_hresult(PropVariantToStringAlloc(pv, &buffer));
                    metadata.AlbumTitle = hstring(buffer);
                    CoTaskMemFree(buffer);
                }
                else if (key == PKEY_Audio_IsVariableBitRate)
                {
                    BOOL val;
                    check_hresult(PropVariantToBoolean(pv, &val));
                    metadata.AudioIsVariableBitrate = val;
                }
                else if (key == PKEY_Video_IsStereo)
                {
                    BOOL val;
                    check_hresult(PropVariantToBoolean(pv, &val));
                    metadata.VideoIsStereo = val;
                }

                check_hresult(PropVariantClear(&pv));
            }
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::GetMetadata: Failed to get media metadata;\n" << e.message() << '\n';

            MediaMetadata mm;
            mm.Path = path;
            return mm;
        }

        return metadata;
    }

    void PlayerService::OnLoaded()
    {
        m_VideoSurfaceHandle = m_MediaEngineWrapper ? m_MediaEngineWrapper->GetSurfaceHandle() : nullptr;

        if (m_VideoSurfaceHandle && m_SwapChainPanel)
        {
            m_SwapChainPanel.DispatcherQueue().TryEnqueue([&]()
                {
                    com_ptr<ISwapChainPanelNative2> panelNative;
                    m_SwapChainPanel.as(panelNative);
                    check_hresult(panelNative->SetSwapChainHandle(m_VideoSurfaceHandle));
                });
        }
    }

    void PlayerService::OnPlaybackEnded()
    {
        Next();
    }

    void PlayerService::OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr)
    {
    }
}
