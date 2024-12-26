#include "pch.h"
#include "PlayerService.h"

#include "mfapi.h"
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
    if (m_Source)
    {
        m_Source->Release();
    }
}

void PlayerService::SetSource(winrt::Windows::Foundation::Uri path)
{
    IMFSourceResolver* sourceResolver = nullptr;
    IUnknown* source = nullptr;

    try
    {
        HRESULT hr;

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
    }
    catch (HRESULT e)
    {
        if (sourceResolver)
        {
            sourceResolver->Release();
        }

        if (source)
        {
            source->Release();
        }

        std::cerr << ("PlayerService::SetSource: Failed to set media source;\nHRESULT: " + std::to_string(e)).c_str();

        return;
    }

    if (sourceResolver)
    {
        sourceResolver->Release();
    }

    if (source)
    {
        source->Release();
    }

    m_Metadata = GetMetadataInternal();
}

bool PlayerService::HasSource()
{
    return m_Source;
}

void PlayerService::Seek(unsigned int time)
{
    m_Position = time;
}

unsigned int PlayerService::GetPosition()
{
    return m_Position;
}

unsigned int PlayerService::GetRemaining()
{
    return m_Metadata ? m_Metadata->duration - m_Position : 0;
}

std::wstring PlayerService::DurationToWString(unsigned int duration)
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
                PropVariantToUInt64(pv, &metadata.duration);
                metadata.duration /= 10000;
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

        return {};
    }

    if (props)
    {
        props->Release();
    }

    return metadata;
}
