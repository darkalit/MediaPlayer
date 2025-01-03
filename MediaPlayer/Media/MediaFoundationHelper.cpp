#include "pch.h"
#include "MediaFoundationHelper.h"

#include "mfapi.h"

MFPlatformRef::~MFPlatformRef()
{
    Shutdown();
}

void MFPlatformRef::Startup()
{
    if (m_Started) return;

    winrt::check_hresult(MFStartup(MF_VERSION, MFSTARTUP_FULL));
    m_Started = true;
}

void MFPlatformRef::Shutdown()
{
    if (!m_Started) return;

    winrt::check_hresult(MFShutdown());
    m_Started = false;
}

MFCallbackBase::MFCallbackBase(DWORD flags, DWORD queue)
    : m_Flags(flags)
    , m_Queue(queue)
{
}

DWORD MFCallbackBase::GetFlags()
{
    return m_Flags;
}

DWORD MFCallbackBase::GetQeueu()
{
    return m_Queue;
}

STDMETHODIMP MFCallbackBase::GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
{
    *pdwFlags = m_Flags;
    *pdwQueue = m_Queue;
    return S_OK;
}

STDMETHODIMP SyncMFCallback::Invoke(IMFAsyncResult* pAsyncResult)
{
    return E_NOTIMPL;
}
