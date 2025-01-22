#pragma once

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;

struct WavHeader
{
    // RIFF chunk descriptor
    uint8_t Riff[4] = { 'R', 'I', 'F', 'F' };
    uint32_t ChunkSize;
    uint8_t Wave[4] = { 'W', 'A', 'V', 'E' };

    // fmt sub-chunk
    uint8_t Fmt[4] = { 'f', 'm', 't', ' ' };
    uint32_t FmtSize = 16;
    uint16_t AudioFormat = 1;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;

    // data sub-chunk
    uint8_t Data[4] = { 'd', 'a', 't', 'a' };
    uint32_t DataSize;
};

class FfmpegDecoder
{
public:
    FfmpegDecoder();
    ~FfmpegDecoder();

    void OpenFile(winrt::hstring const& filepath);
    std::vector<uint8_t>& GetWavBuffer();

private:
    AVFormatContext* m_FormatContext;
    AVCodecContext* m_AudioCodecContext;
    int m_AudioStreamIndex;
    SwrContext* m_SwrContext;

    std::vector<uint8_t> m_WavBuffer;
};

