#pragma once

#include "xaudio2.h"
#include "XAudioStreamCallback.h"
#include "FfmpegDecoder.h"
#include "MediaConfig.h"
#include "SoundTouch.h"

class XAudio2Player
{
public:
    XAudio2Player();
    ~XAudio2Player();

    void Start();
    void Stop();

    void SubmitSample(AudioSample& sample);
    void SetRate(float rate);
    void SetVolume(float volume);
    float GetRate();
    float GetVolume();
    bool IsSampleConsumed();

private:
    void ProcessSample(AudioSample& sample);

    bool m_Running = false;
    IXAudio2* m_XAudio2 = nullptr;
    IXAudio2MasteringVoice* m_MasterVoice = nullptr;
    IXAudio2SourceVoice* m_SourceVoice = nullptr;
    XAudioStreamCallback* m_Callback = nullptr;

    float m_Rate = 1.0f;
    soundtouch::SoundTouch m_SoundTouch;
};

