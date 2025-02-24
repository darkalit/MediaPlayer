#include "pch.h"
#include "XAudio2Player.h"

using namespace winrt;

XAudio2Player::XAudio2Player()
{
    m_Callback = new XAudioStreamCallback();
}

XAudio2Player::~XAudio2Player()
{
    Stop();
    delete m_Callback;
}

void XAudio2Player::SubmitSample(AudioSample& sample)
{
    if (!m_SourceVoice || sample.Buffer.empty())
    {
        return;
    }        

    XAUDIO2_BUFFER buffer = {
        .Flags = XAUDIO2_END_OF_STREAM,
        .AudioBytes = static_cast<UINT32>(sample.Buffer.size()),
        .pAudioData = sample.Buffer.data(),
    };

    m_Callback->WaitForFreeBuffer();
    m_SourceVoice->SubmitSourceBuffer(&buffer);
}

bool XAudio2Player::IsSampleConsumed()
{
    if (!m_SourceVoice) return true;

    XAUDIO2_VOICE_STATE state;
    m_SourceVoice->GetState(&state);
    return state.BuffersQueued == 0;
}

void XAudio2Player::Start()
{
    m_Running = true;

    check_hresult(XAudio2Create(&m_XAudio2));
    check_hresult(m_XAudio2->CreateMasteringVoice(&m_MasterVoice));

    WAVEFORMATEX wfx = {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .wBitsPerSample = 16,
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
