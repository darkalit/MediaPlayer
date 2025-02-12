#pragma once

#include "queue"

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct SwsContext;

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

struct VideoFrame
{
    std::vector<uint8_t> Buffer;
    int Width;
    int Height;
    int RowPitch;
    double FrameTime;
};

enum class SubType
{
    TEXT,
    ASS,
};

struct SubtitleItem
{
    double StartTime;
    double EndTime;
    winrt::hstring Name;
    winrt::hstring Text;
};

class FfmpegDecoder
{
public:
    FfmpegDecoder();
    ~FfmpegDecoder();

    void OpenFile(winrt::hstring const& filepath);
    std::vector<uint8_t>& GetWavBuffer();
    std::queue<SubtitleItem>& GetSubsQueue();
    VideoFrame GetNextFrame();
    void Seek(uint64_t time); // in milliseconds

private:
    static void ParseAssDialogue(SubtitleItem& subItem, const std::string& dialogueEvent);

    AVFormatContext* m_FormatContext;
    AVCodecContext* m_AudioCodecContext;
    AVCodecContext* m_VideoCodecContext;
    AVCodecContext* m_SubtitlesCodecContext;
    int m_AudioStreamIndex;
    int m_VideoStreamIndex;
    int m_SubtitlesStreamIndex;
    SwrContext* m_SwrContext;
    SwsContext* m_SwsContext;

    bool m_FileOpened = false;

    std::vector<uint8_t> m_WavBuffer;
    std::queue<SubtitleItem> m_SubsQueue;
};

