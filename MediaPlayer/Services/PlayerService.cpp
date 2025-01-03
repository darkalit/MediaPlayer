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

PlayerService::PlayerService()
{
    try
    {
        winrt::check_hresult(MFStartup(MF_VERSION));
    }
    catch (const winrt::hresult_error& e)
    {
        std::wcerr << L"PlayerService::PlayerService: Failed to init Media Foundation;\n" << e.message() << '\n';
    }
}

PlayerService::~PlayerService()
{
    try
    {
        winrt::check_hresult(MFShutdown());
    }
    catch (const winrt::hresult_error& e)
    {
        std::wcerr << L"PlayerService::~PlayerService: Failed to shutdown Media Foundation;\n" << e.message() << '\n';
    }
}

void PlayerService::Init(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel, const winrt::Windows::Foundation::Uri& path)
{
    m_DeviceManager = nullptr;
    UINT resetToken = 0;
    winrt::check_hresult(MFLockDXGIDeviceManager(&resetToken, m_DeviceManager.put()));

    winrt::com_ptr<ID3D11Device> d3d11Device;
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

    winrt::check_hresult(D3D11CreateDevice(
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

    winrt::com_ptr<ID3D10Multithread> multithreadedDevice;
    d3d11Device.as(multithreadedDevice);
    multithreadedDevice->SetMultithreadProtected(true);

    winrt::check_hresult(m_DeviceManager->ResetDevice(d3d11Device.get(), resetToken));

    m_SwapChainPanel = panel;
    UINT32 height = 0;
    UINT32 width = 0;
    
    winrt::com_ptr<IMFMediaType> nativeMediaType;

    winrt::check_hresult(MFCreateSourceReaderFromURL(path.ToString().c_str(), nullptr, m_SourceReader.put()));
    winrt::check_hresult(m_SourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nativeMediaType.put()));
    winrt::check_hresult(MFGetAttributeSize(nativeMediaType.get(), MF_MT_FRAME_SIZE, &width, &height));

    auto onLoadedCB = std::bind(&PlayerService::OnLoaded, this);
    auto onPlaybackEndedCB = std::bind(&PlayerService::OnPlaybackEnded, this);
    auto onErrorCB = std::bind(&PlayerService::OnError, this, std::placeholders::_1, std::placeholders::_2);

    m_MediaEngineWrapper = winrt::make_self<MediaEngineWrapper>(
        m_SourceReader.get(),
        m_DeviceManager.get(),
        onLoadedCB,
        onPlaybackEndedCB,
        onErrorCB,
        width,
        height);
}

void PlayerService::SetSource(const winrt::Windows::Foundation::Uri& path, HWND hwnd)
{
}

bool PlayerService::HasSource()
{
    return true;
}

PlayerService::State PlayerService::GetState()
{
    return m_State;
}

void PlayerService::Start(const std::optional<long long>& time)
{
    try
    {
    }
    catch (const winrt::hresult_error& e)
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
    }
    catch (const winrt::hresult_error& e)
    {
        std::wcerr << L"PlayerService::Stop: Failed to stop media;\n" << e.message() << '\n';
    }
}

void PlayerService::Pause()
{
    m_State = State::PAUSED;

    try
    {
    }
    catch (const winrt::hresult_error& e)
    {
        std::wcerr << L"PlayerService::Pause: Failed to stop media;\n" << e.message() << '\n';
    }
}

void PlayerService::Play()
{
    Start();
}

long long PlayerService::GetPosition()
{
    //m_Position = m_MediaEngine->GetCurrentTime() * 1000;

    //winrt::com_ptr<IMFClock> clock;
    //winrt::com_ptr<IMFPresentationClock> presentationClock;

    //try
    //{
    //    if (!m_MediaSession) return 0;

    //    winrt::check_hresult(m_MediaSession->GetClock(clock.put()));
    //    presentationClock = clock.as<IMFPresentationClock>();

    //    LONGLONG llTime = 0;
    //    presentationClock->GetTime(&llTime);
    //    m_Position = llTime / 10000;
    //}
    //catch (winrt::hresult_error const& e)
    //{
    //    std::wcerr << L"PlayerService::GetPosition: Failed to get media playing position;\n" << e.message() << '\n';
    //    return 0;
    //}

    return m_Position;
}

long long PlayerService::GetRemaining()
{
    //return m_Metadata ? m_Metadata->duration - m_Position : 0;
    return 0;
}

winrt::hstring PlayerService::DurationToWString(long long duration)
{
    if (duration == 0)
    {
        return L"00:00:00";
    }

    unsigned int hours = duration / (1000 * 60 * 60);
    unsigned int minutes = (duration / (1000 * 60)) % 60;
    unsigned int seconds = (duration / 1000) % 60;

    return (hours < 10 ? L"0" : L"") + winrt::to_hstring(hours) + L":" + (minutes < 10 ? L"0" : L"") + winrt::to_hstring(minutes) + L":" + (seconds < 10 ? L"0" : L"") + winrt::to_hstring(seconds);
}

std::optional<PlayerService::MediaMetadata> PlayerService::GetMetadata()
{
    //if (HasSource() && m_Metadata)
    //{
    //    return m_Metadata;
    //}
    //else
    //{
        return {};
    //}
}

void PlayerService::OnLoaded()
{
    m_VideoSurfaceHandle = m_MediaEngineWrapper ? m_MediaEngineWrapper->GetSurfaceHandle() : nullptr;

    if (m_VideoSurfaceHandle)
    {
        m_SwapChainPanel.DispatcherQueue().TryEnqueue([=]() {
            winrt::com_ptr<ISwapChainPanelNative2> panelNative;
            m_SwapChainPanel.as(panelNative);
            winrt::check_hresult(panelNative->SetSwapChainHandle(m_VideoSurfaceHandle));
        });
    }

    m_MediaEngineWrapper->Start(0);
}

void PlayerService::OnPlaybackEnded()
{
    MFShutdown();
}

void PlayerService::OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr)
{
}

std::optional<PlayerService::MediaMetadata> PlayerService::GetMetadataInternal()
{
    winrt::com_ptr<IPropertyStore> props;
    MediaMetadata metadata;

    try
    {
        DWORD cProps;

        //winrt::check_hresult(MFGetService(m_Source.get(), MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(props.put())));

        winrt::check_hresult(props->GetCount(&cProps));

        for (DWORD i = 0; i < cProps; i++)
        {
            PROPERTYKEY key;
            PROPVARIANT pv;

            winrt::check_hresult(props->GetAt(i, &key));
            winrt::check_hresult(props->GetValue(key, &pv));

            if (key == PKEY_Media_Duration)
            {
                ULONGLONG duration;
                winrt::check_hresult(PropVariantToUInt64(pv, &duration));
                metadata.duration = duration / 10000;
            }
            else if (key == PKEY_Audio_ChannelCount)
            {
                metadata.audioChannelCount = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.audioChannelCount));
            }
            else if (key == PKEY_Audio_EncodingBitrate)
            {
                metadata.audioBitrate = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.audioBitrate));
            }
            else if (key == PKEY_Audio_SampleRate)
            {
                metadata.audioSampleRate = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.audioSampleRate));
            }
            else if (key == PKEY_Audio_SampleSize)
            {
                metadata.audioSampleSize = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.audioSampleSize));
            }
            else if (key == PKEY_Audio_StreamNumber)
            {
                metadata.audioStreamId = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.audioStreamId));
            }
            else if (key == PKEY_Video_EncodingBitrate)
            {
                metadata.videoBitrate = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.videoBitrate));
            }
            else if (key == PKEY_Video_FrameWidth)
            {
                metadata.videoWidth = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.videoWidth));
            }
            else if (key == PKEY_Video_FrameHeight)
            {
                metadata.videoHeight = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.videoHeight));
            }
            else if (key == PKEY_Video_FrameRate)
            {
                metadata.videoFrameRate = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.videoFrameRate));
                *metadata.videoFrameRate /= 1000;
            }
            else if (key == PKEY_Video_StreamNumber)
            {
                metadata.videoStreamId = 0;
                winrt::check_hresult(PropVariantToUInt32(pv, &*metadata.videoStreamId));
            }
            else if (key == PKEY_Author)
            {
                wchar_t* buffer;
                winrt::check_hresult(PropVariantToStringAlloc(pv, &buffer));
                metadata.author = winrt::hstring(buffer);
                CoTaskMemFree(buffer);
            }
            else if (key == PKEY_Title)
            {
                wchar_t* buffer;
                winrt::check_hresult(PropVariantToStringAlloc(pv, &buffer));
                metadata.title = winrt::hstring(buffer);
                CoTaskMemFree(buffer);
            }
            else if (key == PKEY_Music_AlbumTitle)
            {
                wchar_t* buffer;
                winrt::check_hresult(PropVariantToStringAlloc(pv, &buffer));
                metadata.albumTitle = winrt::hstring(buffer);
                CoTaskMemFree(buffer);
            }
            else if (key == PKEY_Audio_IsVariableBitRate)
            {
                BOOL val;
                winrt::check_hresult(PropVariantToBoolean(pv, &val));
                metadata.audioIsVariableBitrate = val;
            }
            else if (key == PKEY_Video_IsStereo)
            {
                BOOL val;
                winrt::check_hresult(PropVariantToBoolean(pv, &val));
                metadata.videoIsStereo = val;
            }

            winrt::check_hresult(PropVariantClear(&pv));
        }
    }
    catch (const winrt::hresult_error& e)
    {
        std::wcerr << L"PlayerService::GetMetadata: Failed to get media metadata;\n" << e.message() << '\n';

        return {};
    }

    return metadata;
}
