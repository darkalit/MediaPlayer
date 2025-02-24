#pragma once

#include "xaudio2.h"
#include "mutex"

class XAudioStreamCallback : public IXAudio2VoiceCallback
{
public:
    void OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
    void OnVoiceProcessingPassEnd() override;
    void OnStreamEnd() override;
    void OnBufferStart(void* pBufferContext) override;
    void OnBufferEnd(void* pBufferContext) override;
    void OnLoopEnd(void* pBufferContext) override;
    void OnVoiceError(void* pBufferContext, HRESULT Error) override;

    void WaitForFreeBuffer();

private:
    std::mutex m_Mutex;
    std::condition_variable m_CV;
    UINT32 m_BuffersPlayed = 0;
};

