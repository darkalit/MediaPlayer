#include "pch.h"
#include "XAudioStreamCallback.h"

void XAudioStreamCallback::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
}

void XAudioStreamCallback::OnVoiceProcessingPassEnd()
{
}

void XAudioStreamCallback::OnStreamEnd()
{
}

void XAudioStreamCallback::OnBufferStart(void* pBufferContext)
{
}

void XAudioStreamCallback::OnBufferEnd(void* pBufferContext)
{
    delete[] static_cast<float*>(pBufferContext);
    {
        std::lock_guard lock(m_Mutex);
        ++m_BuffersPlayed;
    }
    m_CV.notify_one();
}

void XAudioStreamCallback::OnLoopEnd(void* pBufferContext)
{
}

void XAudioStreamCallback::OnVoiceError(void* pBufferContext, HRESULT Error)
{
}

void XAudioStreamCallback::WaitForFreeBuffer()
{
    std::unique_lock lock(m_Mutex);
    m_CV.wait(lock, [this](){ return m_BuffersPlayed > 0; });
    --m_BuffersPlayed;
}

void XAudioStreamCallback::Reset()
{
    {
        std::lock_guard lock(m_Mutex);
        m_BuffersPlayed = 2;
    }
    m_CV.notify_one();
}
