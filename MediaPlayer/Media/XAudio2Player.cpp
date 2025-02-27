#include "pch.h"
#include "XAudio2Player.h"

#include "MediaConfig.h"

using namespace winrt;

XAudio2Player::XAudio2Player()
{
    m_Callback = new XAudioStreamCallback();
    m_SoundTouch.setSampleRate(MediaConfig::AudioSampleRate);
    m_SoundTouch.setChannels(MediaConfig::AudioChannels);
    m_SoundTouch.setRate(1.0);
}

XAudio2Player::~XAudio2Player()
{
    Stop();
    delete m_Callback;
}

void XAudio2Player::Start()
{
    m_Running = true;

    check_hresult(XAudio2Create(&m_XAudio2));
    check_hresult(m_XAudio2->CreateMasteringVoice(&m_MasterVoice));

    WAVEFORMATEX wfx = {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = MediaConfig::AudioChannels,
        .nSamplesPerSec = MediaConfig::AudioSampleRate,
        .wBitsPerSample = MediaConfig::AudioBitsPerSample,
        .cbSize = 0,
    };

    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    check_hresult(m_XAudio2->CreateSourceVoice(&m_SourceVoice, &wfx, 0, 2.0f, m_Callback));
    m_SourceVoice->Start(0);
}

void XAudio2Player::Stop()
{
    m_Running = false;

    if (m_SourceVoice)
    {
        m_SourceVoice->Stop();
        m_SourceVoice->DestroyVoice();
        m_SourceVoice = nullptr;
    }

    if (m_MasterVoice)
    {
        m_MasterVoice->DestroyVoice();
        m_MasterVoice = nullptr;
    }

    if (m_XAudio2)
    {
        m_XAudio2->Release();
        m_XAudio2 = nullptr;
    }

    if (m_Callback)
    {
        delete m_Callback;
        m_Callback = nullptr;
    }
}

void XAudio2Player::SubmitSample(AudioSample& sample)
{
    if (!m_SourceVoice || sample.Buffer.empty())
    {
        return;
    }

    if (m_Rate != 1.0f)
    {
        ProcessSample(sample);
    }

    auto audioData = new uint8_t[sample.Buffer.size()];
    std::copy(sample.Buffer.begin(), sample.Buffer.end(), audioData);

    XAUDIO2_BUFFER buffer = {
        .Flags = XAUDIO2_END_OF_STREAM,
        .AudioBytes = static_cast<UINT32>(sample.Buffer.size()),
        .pAudioData = audioData,
        .pContext = audioData,
    };

    m_Callback->WaitForFreeBuffer();
    m_SourceVoice->SubmitSourceBuffer(&buffer);
}

void XAudio2Player::SetRate(float rate)
{
    m_Rate = rate;
    m_SoundTouch.setRate(rate);
}

void XAudio2Player::SetVolume(float volume)
{
    if (!m_SourceVoice) return;

    m_SourceVoice->SetVolume(volume);
}

float XAudio2Player::GetRate()
{
    return m_Rate;
}

float XAudio2Player::GetVolume()
{
    if (!m_SourceVoice) return 0.0f;

    float volume;
    m_SourceVoice->GetVolume(&volume);
    return volume;
}

bool XAudio2Player::IsSampleConsumed()
{
    if (!m_SourceVoice) return true;

    XAUDIO2_VOICE_STATE state;
    m_SourceVoice->GetState(&state);
    return state.BuffersQueued == 0;
}

void XAudio2Player::ProcessSample(AudioSample& sample)
{
    m_SoundTouch.putSamples(reinterpret_cast<int16_t*>(sample.Buffer.data()), sample.Buffer.size() / 2);

    double ratio = m_SoundTouch.getInputOutputSampleRatio();
    AudioSample outSample;
    outSample.Buffer = std::vector<uint8_t>(sample.Buffer.size() * ratio);
    outSample.Duration = sample.Duration * ratio;
    outSample.StartTime = sample.StartTime;

    auto outputSize = m_SoundTouch.receiveSamples(reinterpret_cast<int16_t*>(outSample.Buffer.data()), outSample.Buffer.size() / 2);

    m_SoundTouch.flush();

    sample = outSample;
}
