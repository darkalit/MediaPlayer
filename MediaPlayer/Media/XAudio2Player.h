#pragma once

#include "xaudio2.h"
#include "XAudioStreamCallback.h"
#include "FfmpegDecoder.h"

class XAudio2Player
{
public:
    XAudio2Player();
    ~XAudio2Player();

    void SubmitSample(AudioSample& sample);
    bool IsSampleConsumed();

    void Start();
    void Stop();

private:
    bool m_Running = false;
    IXAudio2* m_XAudio2 = nullptr;
    IXAudio2MasteringVoice* m_MasterVoice = nullptr;
    IXAudio2SourceVoice* m_SourceVoice = nullptr;
    XAudioStreamCallback* m_Callback = nullptr;
};

