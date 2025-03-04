#include "pch.h"
#include "XAudio2Player.h"

#include "MediaConfig.h"

using namespace winrt;

XAudio2Player::XAudio2Player()
{
    m_SoundTouch.setSampleRate(MediaConfig::AudioSampleRate);
    m_SoundTouch.setChannels(MediaConfig::AudioChannels);
    m_SoundTouch.setRate(1.0);
}

XAudio2Player::~XAudio2Player()
{
    Stop();
}

void XAudio2Player::Start()
{
    m_Running = true;

    check_hresult(XAudio2Create(&m_XAudio2));
    check_hresult(m_XAudio2->CreateMasteringVoice(&m_MasterVoice));

    WAVEFORMATEX wfx = {
        .wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
        .nChannels = MediaConfig::AudioChannels,
        .nSamplesPerSec = MediaConfig::AudioSampleRate,
        .wBitsPerSample = MediaConfig::AudioBitsPerSample,
        .cbSize = 0,
    };

    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    check_hresult(m_XAudio2->CreateSourceVoice(&m_SourceVoice, &wfx, 0, 2.0f, &m_Callback));
    m_SourceVoice->Start(0);
}

void XAudio2Player::Stop()
{
    m_Running = false;
    m_SoundTouch.flush();

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

    m_Callback.Reset();
}

void XAudio2Player::SubmitSample(AudioSample& sample)
{
    if (!m_SourceVoice || sample.Buffer.empty())
    {
        return;
    }

    ProcessSample(sample);

    auto audioData = new float[sample.Buffer.size()];
    std::copy(sample.Buffer.begin(), sample.Buffer.end(), audioData);

    XAUDIO2_BUFFER buffer = {
        .Flags = XAUDIO2_END_OF_STREAM,
        .AudioBytes = static_cast<UINT32>(sample.Buffer.size() * sizeof(float)),
        .pAudioData = reinterpret_cast<BYTE*>(audioData),
        .pContext = audioData,
    };

    m_Callback.WaitForFreeBuffer();
    m_SourceVoice->SubmitSourceBuffer(&buffer);
}

void XAudio2Player::SetRate(float rate)
{
    m_Rate = rate;
    m_SoundTouch.setTempo(rate);
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
    m_SoundTouch.putSamples(sample.Buffer.data(), sample.Buffer.size() / MediaConfig::AudioChannels);

    AudioSample outSample;
    outSample.Buffer;
    outSample.Duration = sample.Duration;
    outSample.StartTime = sample.StartTime;

    constexpr int BUFFER_CHUNK = 1024;
    std::vector<float> tempBuffer(BUFFER_CHUNK * MediaConfig::AudioChannels);

    do
    {
        int count = m_SoundTouch.receiveSamples(tempBuffer.data(), BUFFER_CHUNK);
        if (count > 0)
        {
            outSample.Buffer.insert(outSample.Buffer.end(), tempBuffer.begin(), tempBuffer.begin() + (count * MediaConfig::AudioChannels));
        }
        else break;
    } while (true);

    sample = outSample;
}
