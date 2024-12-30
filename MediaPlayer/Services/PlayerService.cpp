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

void PlayerService::SetSource(const winrt::Windows::Foundation::Uri& path)
{
    winrt::com_ptr<IMFSourceResolver> sourceResolver;
    winrt::com_ptr<IUnknown> source;
    winrt::com_ptr<IMFTopology> topology;
    winrt::com_ptr<IMFActivate> audioRendererActivate;
    winrt::com_ptr<IMFTopologyNode> sourceNode;
    winrt::com_ptr<IMFTopologyNode> outputNode;
    winrt::com_ptr<IMFPresentationDescriptor> presentationDescriptor;
    winrt::com_ptr<IMFStreamDescriptor> streamDescriptor;

    try
    {
        if (m_MediaSession)
        {
            winrt::check_hresult(m_MediaSession->Close());
        }

        if (m_Source)
        {
            winrt::check_hresult(m_Source->Shutdown());
        }

        if (m_MediaSession)
        {
            winrt::check_hresult(m_MediaSession->Shutdown());
        }

        winrt::check_hresult(MFCreateSourceResolver(sourceResolver.put()));

        MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
        winrt::check_hresult(sourceResolver->CreateObjectFromURL(
            path.ToString().c_str(),
            MF_RESOLUTION_MEDIASOURCE,
            nullptr,
            &objectType,
            source.put()
        ));

        m_Source = source.as<IMFMediaSource>();

        m_State = State::CLOSED;

        winrt::check_hresult(MFCreateMediaSession(nullptr, m_MediaSession.put()));

        m_State = State::READY;

        winrt::check_hresult(MFCreateTopology(topology.put()));

        winrt::check_hresult(m_Source->CreatePresentationDescriptor(presentationDescriptor.put()));
        BOOL streamSelected = 0;
        winrt::check_hresult(presentationDescriptor->GetStreamDescriptorByIndex(0, &streamSelected, streamDescriptor.put()));

        WINRT_ASSERT(streamSelected && streamDescriptor);

        winrt::check_hresult(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, sourceNode.put()));
        winrt::check_hresult(sourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_Source.get()));
        winrt::check_hresult(sourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, presentationDescriptor.get()));
        winrt::check_hresult(sourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, streamDescriptor.get()));
        winrt::check_hresult(topology->AddNode(sourceNode.get()));

        winrt::check_hresult(MFCreateAudioRendererActivate(audioRendererActivate.put()));
        winrt::check_hresult(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, outputNode.put()));
        winrt::check_hresult(outputNode->SetObject(audioRendererActivate.get()));
        winrt::check_hresult(topology->AddNode(outputNode.get()));
        winrt::check_hresult(sourceNode->ConnectOutput(0, outputNode.get(), 0));
        winrt::check_hresult(m_MediaSession->SetTopology(0, topology.get()));

        m_State = State::STOPPED;

        m_Metadata = GetMetadataInternal();
    }
    catch (const winrt::hresult_error& e)
    {
        std::wcerr << L"PlayerService::SetSource: Failed to set media source;\n" << e.message() << '\n';
    }
}

bool PlayerService::HasSource()
{
    return !!m_Source;
}

PlayerService::State PlayerService::GetState()
{
    return m_State;
}

void PlayerService::Start(const std::optional<long long>& time)
{
    if (!m_MediaSession) return;

    try
    {
        if (time)
        {
            m_Position = *time;
        }

        PROPVARIANT start;
        PropVariantInit(&start);
        start.vt = VT_I8;
        start.hVal.QuadPart = (time.has_value() ? *time : GetPosition()) * 10000;

        winrt::check_hresult(m_MediaSession->Start(&GUID_NULL, &start));

        PropVariantClear(&start);
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
        winrt::check_hresult(m_MediaSession->Stop());
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
        m_MediaSession->Pause();
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
    winrt::com_ptr<IMFClock> clock;
    winrt::com_ptr<IMFPresentationClock> presentationClock;

    try
    {
        if (!m_MediaSession) return 0;

        winrt::check_hresult(m_MediaSession->GetClock(clock.put()));
        presentationClock = clock.as<IMFPresentationClock>();

        LONGLONG llTime = 0;
        presentationClock->GetTime(&llTime);
        m_Position = llTime / 10000;
    }
    catch (winrt::hresult_error const& e)
    {
        std::wcerr << L"PlayerService::GetPosition: Failed to get media playing position;\n" << e.message() << '\n';
        return 0;
    }

    return m_Position;
}

long long PlayerService::GetRemaining()
{
    return m_Metadata ? m_Metadata->duration - m_Position : 0;
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
    winrt::com_ptr<IPropertyStore> props;
    MediaMetadata metadata;

    try
    {
        DWORD cProps;

        winrt::check_hresult(MFGetService(m_Source.get(), MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(props.put())));

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
