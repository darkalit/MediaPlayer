#include "pch.h"
#include "MediaEngineNotifyImpl.h"

#include "Mferror.h"

MediaEngineNotifyImpl::MediaEngineNotifyImpl(
    std::function<void()> onLoadedCB,
    std::function<void()> onPlaybackEndedCB,
    std::function<void(MF_MEDIA_ENGINE_ERR, HRESULT)> onErrorCB)
    : m_OnLoadedCB(onLoadedCB)
    , m_OnPlaybackEndedCB(onPlaybackEndedCB)
    , m_OnErrorCB(onErrorCB)
{
}

STDMETHODIMP MediaEngineNotifyImpl::EventNotify(DWORD event, DWORD_PTR param1, DWORD param2)
{
    if (m_Detached) return MF_E_SHUTDOWN;

    switch (static_cast<MF_MEDIA_ENGINE_EVENT>(event))
    {
    case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
        m_OnLoadedCB();
        break;

    case MF_MEDIA_ENGINE_EVENT_ENDED:
        m_OnPlaybackEndedCB();
        break;

    case MF_MEDIA_ENGINE_EVENT_ERROR:
        m_OnErrorCB(static_cast<MF_MEDIA_ENGINE_ERR>(param1), static_cast<HRESULT>(param2));
        break;

    default:
        break;
    }

    return S_OK;
}

void MediaEngineNotifyImpl::Shutdown()
{
    m_Detached = true;
    m_OnLoadedCB = nullptr;
    m_OnPlaybackEndedCB = nullptr;
    m_OnErrorCB = nullptr;
}
