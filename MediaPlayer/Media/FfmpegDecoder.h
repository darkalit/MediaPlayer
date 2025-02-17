#pragma once

#include <winrt/MediaPlayer.h>

#include "Framework/SharedQueue.h"

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct SwsContext;
struct AVPacket;

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

//struct SubtitleStream
//{
//    int Index;
//    winrt::hstring Language;
//    winrt::hstring Title;
//};

class FfmpegDecoder
{
public:
    FfmpegDecoder();
    ~FfmpegDecoder();

    void OpenFile(winrt::hstring const& filepath);
    void OpenSubtitle(winrt::hstring const& filepath);
    void OpenSubtitle(unsigned int subtitleIndex);
    void SetupDecoding(SharedQueue<VideoFrame>& frames, SharedQueue<SubtitleItem>& subs);
    void PauseDecoding(bool pause);
    std::vector<uint8_t>& GetWavBuffer();
    std::vector<winrt::MediaPlayer::SubtitleStream>& GetSubtitleStreams();
    VideoFrame GetNextFrame();
    void Seek(uint64_t time); // in milliseconds

private:
    void GetSubtitles(AVFormatContext* formatContext);
    int PushSubtitle(SharedQueue<SubtitleItem>& subs, AVFormatContext* formatContext, AVPacket* packet);
    static void ParseAssDialogue(SubtitleItem& subItem, const std::string& dialogueEvent);

    AVFormatContext* m_FormatContext;
    AVFormatContext* m_SubtitleFormatContext = nullptr;
    AVCodecContext* m_AudioCodecContext;
    AVCodecContext* m_VideoCodecContext;
    AVCodecContext* m_SubtitlesCodecContext;
    int m_AudioStreamIndex;
    int m_VideoStreamIndex;
    std::vector<winrt::MediaPlayer::SubtitleStream> m_SubtitleStreams;
    int m_CurrentSubSteamIndex;
    SwrContext* m_SwrContext;
    SwsContext* m_SwsContext;

    bool m_FileOpened = false;
    bool m_DecodingPaused = false;

    std::thread m_DecodingThread;

    std::vector<uint8_t> m_WavBuffer;
};

