#pragma once

#include "mfmediaengine.h"

class MediaEngineExtension : public winrt::implements<MediaEngineExtension, IMFMediaEngineExtension>
{
public:
    MediaEngineExtension() = default;
    ~MediaEngineExtension() = default;

    STDMETHODIMP CanPlayType(BOOL isAudioOnly, BSTR MimeType, MF_MEDIA_ENGINE_CANPLAY* pAnswer) override;
    STDMETHODIMP BeginCreateObject(BSTR bstrURL, IMFByteStream* pByteStream, MF_OBJECT_TYPE type, IUnknown** ppIUnknownCancelCookie, IMFAsyncCallback* pCallback, IUnknown* punkState) override;
    STDMETHODIMP CancelObjectCreation(IUnknown* pIUnknownCancelCookie) override;
    STDMETHODIMP EndCreateObject(IMFAsyncResult* pResult, IUnknown** ppObject) override;

    void SetMediaSource(IUnknown* mediaSource);
    void Shutdown();

private:
    enum class ExtensionUriType
    {
        UNKNOWN = 0,
        CUSTOM_SOURCE
    };
    ExtensionUriType m_UriType = ExtensionUriType::UNKNOWN;
    winrt::com_ptr<IUnknown> m_MediaSource;
    bool m_HasShutdown = false;
};

