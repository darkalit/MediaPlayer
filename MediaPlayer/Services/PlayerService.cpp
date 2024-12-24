#include "pch.h"
#include "PlayerService.h"

#include "mfapi.h"
#include "mfobjects.h"
#include "mfidl.h"
#include "propvarutil.h"
#include "shlwapi.h"
#include "string"
#include "sstream"

PlayerService::PlayerService()
{
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        throw std::exception(("PlayerService::PlayerService: Failed to init Media Foundation; HRESULT: " + std::to_string(hr)).c_str());
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

    }

    if (sourceResolver)
    {
        sourceResolver->Release();
    }

    if (source)
    {
        source->Release();
    }
}

std::wstring PlayerService::GetMetadata()
{
    IPropertyStore* props = nullptr;
    wchar_t buffer[1024] = { 0 };
    std::wostringstream wosstream;
    
    try {
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

            hr = PropVariantToString(pv, buffer, 1024);
            if (FAILED(hr)) throw hr;

            wosstream << std::wstring(buffer) << L"\n";

            PropVariantClear(&pv);
        }
    }
    catch (HRESULT e)
    {

    }

    if (props)
    {
        props->Release();
    }

    return wosstream.str();
}
