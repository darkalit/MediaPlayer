#pragma once

#include <winrt/MediaPlayer.h>

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
}

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
    double StartTime;
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

struct AudioSample
{
    std::vector<uint8_t> Buffer;
    double Duration;
    double StartTime;
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

    bool HasSource();
    static winrt::MediaPlayer::MediaMetadata GetMetadata(winrt::hstring const& filepath);
    void OpenFile(winrt::hstring const& filepath);
    void OpenSubtitle(winrt::hstring const& filepath);
    void OpenSubtitle(unsigned int subtitleIndex);
    void SetupDecoding(SharedQueue<VideoFrame>& frames, SharedQueue<SubtitleItem>& subs, SharedQueue<AudioSample>& audioSamples);
    void PauseDecoding(bool pause);
    uint64_t GetPosition();
    std::vector<winrt::MediaPlayer::SubtitleStream>& GetSubtitleStreams();
    void Seek(uint64_t time); // in milliseconds

private:
    void GetSubtitles(AVFormatContext* formatContext);
    int PushSubtitle(SharedQueue<SubtitleItem>& subs, AVFormatContext* formatContext, AVPacket* packet);
    static void ParseAssDialogue(SubtitleItem& subItem, const std::string& dialogueEvent);

    AVFormatContext* m_FormatContext = nullptr;
    AVFormatContext* m_SubtitleFormatContext = nullptr;
    AVCodecContext* m_AudioCodecContext = nullptr;
    AVCodecContext* m_VideoCodecContext = nullptr;
    AVCodecContext* m_SubtitlesCodecContext = nullptr;
    AVChannelLayout m_ChannelLayout;
    AVSampleFormat m_SampleFormat;
    int m_AudioStreamIndex = -1;
    int m_VideoStreamIndex = -1;
    std::vector<winrt::MediaPlayer::SubtitleStream> m_SubtitleStreams;
    int m_CurrentSubSteamIndex;
    uint64_t m_CurrentTime; // milliseconds
    SwrContext* m_SwrContext;
    SwsContext* m_SwsContext;

    bool m_FileOpened = false;
    bool m_DecodingPaused = false;

    std::thread m_DecodingThread;
};

