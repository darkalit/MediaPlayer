#pragma once

#include "mfobjects.h"

class MFPlatformRef
{
public:
    MFPlatformRef() = default;
    virtual ~MFPlatformRef();

    void Startup();
    void Shutdown();

private:
    bool m_Started;
};

class MFCallbackBase : public winrt::implements<MFCallbackBase, IMFAsyncCallback>
{
public:
    MFCallbackBase(DWORD flags = 0, DWORD queue = MFASYNC_CALLBACK_QUEUE_MULTITHREADED);

    DWORD GetFlags();
    DWORD GetQeueu();

    STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue) override;

private:
    DWORD m_Flags = 0;
    DWORD m_Queue = 0;
};

class SyncMFCallback : public MFCallbackBase
{
public:
    SyncMFCallback();

    void Wait(unsigned int timeout = INFINITE)
    {

    }

    IMFAsyncResult* GetResult();
    STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult) override;

private:
    winrt::com_ptr<IMFAsyncResult> m_Result;

};

