#include "pch.h"
#include "MediaEngineExtension.h"

#include "mfapi.h"
#include "Mferror.h"

STDMETHODIMP MediaEngineExtension::CanPlayType(BOOL isAudioOnly, BSTR MimeType, MF_MEDIA_ENGINE_CANPLAY* pAnswer)
{
    *pAnswer = MF_MEDIA_ENGINE_CANPLAY_NOT_SUPPORTED;
    return S_OK;
}

STDMETHODIMP MediaEngineExtension::BeginCreateObject(BSTR bstrURL, IMFByteStream* pByteStream, MF_OBJECT_TYPE type, IUnknown** ppIUnknownCancelCookie, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
    if (ppIUnknownCancelCookie)
    {
        *ppIUnknownCancelCookie = nullptr;
    }

    winrt::com_ptr<IUnknown> localSource;
    
    if (m_HasShutdown) return MF_E_SHUTDOWN;
    localSource = m_MediaSource;

    if (type == MF_OBJECT_MEDIASOURCE && localSource)
    {
        winrt::com_ptr<IMFAsyncResult> asyncResult;
        winrt::check_hresult(MFCreateAsyncResult(localSource.get(), pCallback, punkState, asyncResult.put()));
        winrt::check_hresult(asyncResult->SetStatus(S_OK));
        m_UriType = ExtensionUriType::CUSTOM_SOURCE;
        winrt::check_hresult(pCallback->Invoke(asyncResult.get()));
    }
    else
    {
        return MF_E_UNEXPECTED;
    }

    return S_OK;
}

STDMETHODIMP MediaEngineExtension::CancelObjectCreation(IUnknown* pIUnknownCancelCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP MediaEngineExtension::EndCreateObject(IMFAsyncResult* pResult, IUnknown** ppObject)
{
    *ppObject = nullptr;
    if (m_UriType == ExtensionUriType::CUSTOM_SOURCE)
    {
        winrt::check_hresult(pResult->GetStatus());
        winrt::check_hresult(pResult->GetObject(ppObject));
        m_UriType = ExtensionUriType::UNKNOWN;
    }
    else
    {
        return MF_E_UNEXPECTED;
    }

    return S_OK;
}

void MediaEngineExtension::SetMediaSource(IUnknown* mediaSource)
{
    if (m_HasShutdown) return;
    m_MediaSource.copy_from(mediaSource);
}

void MediaEngineExtension::Shutdown()
{
    if (!m_HasShutdown)
    {
        m_MediaSource = nullptr;
        m_HasShutdown = true;
    }
}
