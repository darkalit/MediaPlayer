#include "pch.h"
#include "PlayerService.h"

#include "mfapi.h"
#include "mfplay.h"
#include "mfobjects.h"
#include "mfidl.h"
#include "propvarutil.h"
#include "propkeydef.h"
#include "propkey.h"
#include "shlwapi.h"

#include "string"
#include "iostream"

PlayerService::PlayerService()
{
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        throw std::exception(("PlayerService::PlayerService: Failed to init Media Foundation;\nHRESULT: " + std::to_string(hr)).c_str());
    }
}

PlayerService::~PlayerService()
{    
    SafeRelease(m_Source);
    SafeRelease(m_MediaSession);

    MFShutdown();
}

void PlayerService::SetSource(const winrt::Windows::Foundation::Uri& path)
{
    IMFSourceResolver* sourceResolver = nullptr;
    IUnknown* source = nullptr;
    IMFTopology* topology = nullptr;
    IMFActivate* audioRendererActivate = nullptr;
    IMFTopologyNode* sourceNode = nullptr;
    IMFTopologyNode* outputNode = nullptr;
    IMFPresentationDescriptor* presentationDescriptor = nullptr;
    IMFStreamDescriptor* streamDescriptor = nullptr;

    try
    {
        HRESULT hr = S_OK;

        if (m_MediaSession)
        {
            hr = m_MediaSession->Close();
        }

        if (FAILED(hr)) throw hr;

        if (m_Source)
        {
            m_Source->Shutdown();
        }

        if (m_MediaSession)
        {
            m_MediaSession->Shutdown();
        }

        SafeRelease(m_Source);
        SafeRelease(m_MediaSession);

        hr = MFCreateSourceResolver(&sourceResolver);
        if (FAILED(hr)) throw hr;

        MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
        hr = sourceResolver->CreateObjectFromURL(
            path.ToString().c_str(),
            MF_RESOLUTION_MEDIASOURCE,
            nullptr,
            &objectType,
            &source
        );
        if (FAILED(hr)) throw hr;

        hr = source->QueryInterface(IID_PPV_ARGS(&m_Source));
        if (FAILED(hr)) throw hr;

        m_State = State::CLOSED;

        hr = MFCreateMediaSession(nullptr, &m_MediaSession);
        if (FAILED(hr)) throw hr;

        m_State = State::READY;

        hr = MFCreateTopology(&topology);
        if (FAILED(hr)) throw hr;

        m_Source->CreatePresentationDescriptor(&presentationDescriptor);
        BOOL streamSelected = 0;
        hr = presentationDescriptor->GetStreamDescriptorByIndex(0, &streamSelected, &streamDescriptor);

        hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &sourceNode);
        if (FAILED(hr)) throw hr;

        if (streamSelected)
        {
            hr = sourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_Source);
            hr = sourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, presentationDescriptor);
            hr = sourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, streamDescriptor);
        }
        topology->AddNode(sourceNode);

        hr = MFCreateAudioRendererActivate(&audioRendererActivate);
        if (FAILED(hr)) throw hr;

        hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &outputNode);
        if (FAILED(hr)) throw hr;

        hr = outputNode->SetObject(audioRendererActivate);
        if (FAILED(hr)) throw hr;

        topology->AddNode(outputNode);

        sourceNode->ConnectOutput(0, outputNode, 0);        

        hr = m_MediaSession->SetTopology(0, topology);
        if (FAILED(hr)) throw hr;

        m_State = State::STOPPED;

        m_Metadata = GetMetadataInternal();
    }
    catch (HRESULT e)
    {
        std::cerr << ("PlayerService::SetSource: Failed to set media source;\nHRESULT: " + std::to_string(e)).c_str();
    }

    SafeRelease(sourceResolver);
    SafeRelease(source);
    SafeRelease(topology);
    SafeRelease(audioRendererActivate);
    SafeRelease(sourceNode);
    SafeRelease(outputNode);
    SafeRelease(presentationDescriptor);
    SafeRelease(streamDescriptor);
}

bool PlayerService::HasSource()
{
    return m_Source;
}

PlayerService::State PlayerService::GetState()
{
    return m_State;
}

void PlayerService::Start()
{
    if (!m_MediaSession) return;

    PROPVARIANT start;
    PropVariantInit(&start);
    start.vt = VT_I8;
    start.hVal.QuadPart = GetPosition() * 10000;

    m_MediaSession->Start(&GUID_NULL, &start);

    PropVariantClear(&start);

    m_State = State::PLAYING;
}

void PlayerService::Stop()
{
    m_State = State::STOPPED;
    m_MediaSession->Stop();
}

void PlayerService::Pause()
{
    m_State = State::PAUSED;
    m_MediaSession->Pause();
}

void PlayerService::Play()
{
    Start();
}

void PlayerService::Seek(long long time)
{
    m_Position = time;
}

long long PlayerService::GetPosition()
{
    IMFClock* clock = nullptr;
    IMFPresentationClock* presentationClock = nullptr;

    try
    {
        HRESULT hr;

        hr = m_MediaSession->GetClock(&clock);
        if (FAILED(hr)) throw hr;

        hr = clock->QueryInterface(IID_PPV_ARGS(&presentationClock));
        if (FAILED(hr)) throw hr;

        LONGLONG llTime = 0;
        hr = presentationClock->GetTime(&llTime);
        if (FAILED(hr)) throw hr;

        m_Position = llTime / 10000;
    }
    catch (HRESULT e)
    {
        std::cerr << ("PlayerService::GetPosition: Failed to get media playing position;\nHRESULT: " + std::to_string(e)).c_str();
    }

    SafeRelease(clock);
    SafeRelease(presentationClock);

    return m_Position;
}

long long PlayerService::GetRemaining()
{
    return m_Metadata ? m_Metadata->duration - m_Position : 0;
}

std::wstring PlayerService::DurationToWString(long long duration)
{
    if (duration == 0)
    {
        return L"00:00:00";
    }

    unsigned int hours = duration / (1000 * 60 * 60);
    unsigned int minutes = (duration / (1000 * 60)) % 60;
    unsigned int seconds = (duration / 1000) % 60;

    return (hours < 10 ? L"0" : L"") + std::to_wstring(hours) + L":" + (minutes < 10 ? L"0" : L"") + std::to_wstring(minutes) + L":" + (seconds < 10 ? L"0" : L"") + std::to_wstring(seconds);
}

std::optional<PlayerService::MediaMetadata> PlayerService::GetMetadata()
{
    if (HasSource() && m_Metadata)
    {
        return m_Metadata;
    }
    else
    {
        return {};
    }
}

std::optional<PlayerService::MediaMetadata> PlayerService::GetMetadataInternal()
{
    IPropertyStore* props = nullptr;
    MediaMetadata metadata;

    try
    {
        HRESULT hr;
        DWORD cProps;

        hr = MFGetService(m_Source, MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(&props));
        if (FAILED(hr)) throw hr;

        hr = props->GetCount(&cProps);
        if (FAILED(hr)) throw hr;

        for (DWORD i = 0; i < cProps; i++)
        {
            PROPERTYKEY key;
            PROPVARIANT pv;

            hr = props->GetAt(i, &key);
            if (FAILED(hr)) throw hr;

            hr = props->GetValue(key, &pv);
            if (FAILED(hr)) throw hr;

            if (key == PKEY_Media_Duration)
            {
                ULONGLONG duration;
                PropVariantToUInt64(pv, &duration);
                metadata.duration = duration / 10000;
            }
            else if (key == PKEY_Audio_ChannelCount)
            {
                metadata.audioChannelCount = 0;
                PropVariantToUInt32(pv, &*metadata.audioChannelCount);
            }
            else if (key == PKEY_Audio_EncodingBitrate)
            {
                metadata.audioBitrate = 0;
                PropVariantToUInt32(pv, &*metadata.audioBitrate);
            }
            else if (key == PKEY_Audio_SampleRate)
            {
                metadata.audioSampleRate = 0;
                PropVariantToUInt32(pv, &*metadata.audioSampleRate);
            }
            else if (key == PKEY_Audio_SampleSize)
            {
                metadata.audioSampleSize = 0;
                PropVariantToUInt32(pv, &*metadata.audioSampleSize);
            }
            else if (key == PKEY_Audio_StreamNumber)
            {
                metadata.audioStreamId = 0;
                PropVariantToUInt32(pv, &*metadata.audioStreamId);
            }
            else if (key == PKEY_Video_EncodingBitrate)
            {
                metadata.videoBitrate = 0;
                PropVariantToUInt32(pv, &*metadata.videoBitrate);
            }
            else if (key == PKEY_Video_FrameWidth)
            {
                metadata.videoWidth = 0;
                PropVariantToUInt32(pv, &*metadata.videoWidth);
            }
            else if (key == PKEY_Video_FrameHeight)
            {
                metadata.videoHeight = 0;
                PropVariantToUInt32(pv, &*metadata.videoHeight);
            }
            else if (key == PKEY_Video_FrameRate)
            {
                metadata.videoFrameRate = 0;
                PropVariantToUInt32(pv, &*metadata.videoFrameRate);
                *metadata.videoFrameRate /= 1000;
            }
            else if (key == PKEY_Video_StreamNumber)
            {
                metadata.videoStreamId = 0;
                PropVariantToUInt32(pv, &*metadata.videoStreamId);
            }
            else if (key == PKEY_Author)
            {
                size_t bufferSize = wcslen(pv.pwszVal) + 1;
                metadata.author = std::wstring(bufferSize, L'=');
                PropVariantToString(pv, metadata.author->data(), bufferSize);
            }
            else if (key == PKEY_Title)
            {
                size_t bufferSize = wcslen(pv.pwszVal) + 1;
                metadata.title = std::wstring(bufferSize, L'=');
                PropVariantToString(pv, metadata.title->data(), bufferSize);
            }
            else if (key == PKEY_Music_AlbumTitle)
            {
                size_t bufferSize = wcslen(pv.pwszVal) + 1;
                metadata.albumTitle = std::wstring(bufferSize, L'=');
                PropVariantToString(pv, metadata.albumTitle->data(), bufferSize);
            }
            else if (key == PKEY_Audio_IsVariableBitRate)
            {
                BOOL val;
                PropVariantToBoolean(pv, &val);
                metadata.audioIsVariableBitrate = val;
            }
            else if (key == PKEY_Video_IsStereo)
            {
                BOOL val;
                PropVariantToBoolean(pv, &val);
                metadata.videoIsStereo = val;
            }

            PropVariantClear(&pv);
        }
    }
    catch (HRESULT e)
    {
        std::cerr << ("PlayerService::GetMetadata: Failed to get media metadata;\nHRESULT: " + std::to_string(e)).c_str();

        SafeRelease(props);

        return {};
    }

    SafeRelease(props);

    return metadata;
}
