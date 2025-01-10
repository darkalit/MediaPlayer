#include "pch.h"
#include "PlayerService.h"

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
#include <microsoft.ui.xaml.media.dxinterop.h>
#include <d3d11.h>

using namespace winrt::Windows::Foundation;

namespace winrt::MediaPlayer
{
    PlayerService::PlayerService()
    {
        try
        {
            check_hresult(MFStartup(MF_VERSION));
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

        m_State = State::READY;
    }

    void PlayerService::SetSwapChainPanel(Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel)
    {
        m_SwapChainPanel = panel;
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
    }

    void PlayerService::AddSource(const hstring& path, const hstring& displayName)
    {
        auto res = GetMetadataInternal(path);
        
        if (res.Title.empty())
        {
            res.Title = displayName;
        }

        m_MediaPlaylist.Append(res);

        if (m_CurrentMedia == -1)
        {
            m_CurrentMedia = 0;
        }

        if (!HasSource())
        {
            SetSource(path);
        }
    }

    void PlayerService::SetSource(const hstring& path)
    {
        check_hresult(MFCreateSourceReaderFromURL(path.c_str(), nullptr, m_SourceReader.put()));
        m_MediaEngineWrapper->SetSource(m_SourceReader.get());
        if (m_SwapChainPanel) {
            m_SwapChainPanel.DispatcherQueue().TryEnqueue([&]()
            {
                auto size = m_SwapChainPanel.ActualSize();
                m_MediaEngineWrapper->WindowUpdate(size.x, size.y);
            });
        }
        m_Position = 0;
        m_State = State::STOPPED;
    }

    bool PlayerService::HasSource()
    {
        return !!m_SourceReader && m_MediaEngineWrapper && m_MediaPlaylist.Size() > 0 && m_CurrentMedia > -1;
    }

    PlayerService::State PlayerService::GetState()
    {
        return m_State;
    }

    void PlayerService::Next()
    {
        if (!HasSource()) return;

        Stop();
        ++m_CurrentMedia;
        m_CurrentMedia %= m_MediaPlaylist.Size();
        SetSource(m_MediaPlaylist.GetAt(m_CurrentMedia).Path);
        Start();
    }

    void PlayerService::Prev()
    {
        if (!HasSource()) return;

        Stop();
        if (m_CurrentMedia == 0)
        {
            m_CurrentMedia = m_MediaPlaylist.Size();
        }
        --m_CurrentMedia;
        SetSource(m_MediaPlaylist.GetAt(m_CurrentMedia).Path);
        Start();
    }

    void PlayerService::StartByIndex(int index)
    {
        if (!HasSource()) return;

        m_CurrentMedia = max(0, min(m_MediaPlaylist.Size() - 1, index));
        SetSource(m_MediaPlaylist.GetAt(m_CurrentMedia).Path);
        Start();
    }

    void PlayerService::Clear()
    {
        if (!HasSource()) return;

        Stop();
        m_MediaPlaylist.Clear();
        m_CurrentMedia = -1;
    }

    int PlayerService::GetCurrentMediaIndex()
    {
        return m_CurrentMedia;
    }

    void PlayerService::Start(const std::optional<long long>& time)
    {
        if (!HasSource()) return;

        try
        {
            if (time)
            {
                m_Position = *time;
            }

            if (m_CurrentMedia > -1 && m_Position >= m_MediaPlaylist.GetAt(m_CurrentMedia).Duration)
            {
                Stop();
                return;
            }

            double position = static_cast<double>(m_Position) / 1000.0;
            m_MediaEngineWrapper->Start(position);
            SetPlaybackSpeed(m_PlaybackSpeed);
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Start: Failed to start media;\n" << e.message() << '\n';
        }

        m_State = State::PLAYING;
    }

    void PlayerService::Stop()
    {
        m_State = State::STOPPED;

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
        m_State = State::PAUSED;

        try
        {
            m_MediaEngineWrapper->Pause();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Pause: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::SetPlaybackSpeed(double speed)
    {
        m_PlaybackSpeed = speed;
        m_MediaEngineWrapper->SetPlaybackSpeed(speed);
    }

    void PlayerService::SetVolume(double volume)
    {
        m_MediaEngineWrapper->SetVolume(volume);
    }

    double PlayerService::GetVolume()
    {
        return m_MediaEngineWrapper->GetVolume();
    }

    void PlayerService::ResizeVideo(unsigned int width, unsigned int height)
    {
        if (!m_MediaEngineWrapper) return;
        m_MediaEngineWrapper->WindowUpdate(width, height);
    }

    long long PlayerService::GetPosition()
    {
        m_Position = m_MediaEngineWrapper->GetCurrentTime() * 1000;

        return m_Position;
    }

    long long PlayerService::GetRemaining()
    {
        auto metadata = GetMetadata();
        return metadata.Duration ? metadata.Duration - m_Position : 0;
    }

    hstring PlayerService::DurationToWString(long long duration)
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

    MediaMetadata PlayerService::GetMetadata()
    {
        if (HasSource())
        {
            return m_MediaPlaylist.GetAt(m_CurrentMedia);
        }

        return {};
    }

    Collections::IVector<MediaMetadata> PlayerService::GetPlaylist()
    {
        return m_MediaPlaylist;
    }

    MediaMetadata PlayerService::GetMetadataInternal(
        const hstring& path)
    {
        com_ptr<IPropertyStore> props;
        com_ptr<IMFSourceResolver> sourceResolver;
        com_ptr<::IUnknown> source;
        MediaMetadata metadata;

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
