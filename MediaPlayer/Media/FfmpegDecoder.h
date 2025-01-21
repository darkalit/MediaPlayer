#pragma once

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;

class FfmpegDecoder
{
public:
    FfmpegDecoder();
    ~FfmpegDecoder();

    void OpenFile(winrt::hstring const& filepath);
    bool DecodeNextFrame(uint8_t*& outBuffer, int& outSize);

private:
    AVFormatContext* m_FormatContext;
    AVCodecContext* m_AudioCodecContext;
    int m_AudioStreamIndex;
    SwrContext* m_SwrContext;
};

