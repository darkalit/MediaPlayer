#pragma once

#include <winrt/MediaPlayer.h>

#include "functional"

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avutil.h"=
}

#include "Framework/SharedQueue.h"

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct SwsContext;
struct AVPacket;
struct AVFilterGraph;
struct AVFilterContext;

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
    std::vector<float> Buffer;
    double Duration;
    double StartTime;
};

//struct SubtitleStream
//{
//    int Index;
//    winrt::hstring Language;
//    winrt::hstring Title;
//};

enum MediaType : uint8_t
{
    AUDIO = 1 << 0,
    VIDEO = 1 << 1,
    BOTH = AUDIO | VIDEO,
};

class FfmpegDecoder
{
public:
    FfmpegDecoder(std::function<void()> onPlaybackEndedCB);
    ~FfmpegDecoder();

    bool HasSource();
    static winrt::MediaPlayer::MediaMetadata GetMetadata(winrt::hstring const& filepath, MediaType mediaType = MediaType::BOTH);
    void OpenByStreams(winrt::hstring const& video, winrt::hstring const& audio);
    void OpenFile(winrt::hstring const& filepath);
    void OpenSubtitle(winrt::hstring const& filepath);
    void OpenSubtitle(unsigned int subtitleIndex);
    void SetupDecoding(SharedQueue<VideoFrame>& frames, SharedQueue<SubtitleItem>& subs, SharedQueue<AudioSample>& audioSamples);
    void PauseDecoding(bool pause);
    uint64_t GetPosition();
    std::vector<winrt::MediaPlayer::SubtitleStream>& GetSubtitleStreams();
    void Seek(uint64_t time); // in milliseconds
    void SetAudioFilter(winrt::hstring const& filterStr);
    static void RecordSegment(winrt::hstring const& filepath, uint64_t start, uint64_t end);
    static VideoFrame GetFrame(winrt::hstring const& filepath, uint64_t pos, int height = 240);
    static void FfmpegLogError(int error, winrt::hstring const& message);

private:
    void Free();
    void FreeFilters();
    void InitAudioFilter(winrt::hstring const& filterStr);
    void GetSubtitles(AVFormatContext* formatContext);
    int PushSubtitle(SharedQueue<SubtitleItem>& subs, AVFormatContext* formatContext, AVPacket* packet);
    static void ParseAssDialogue(SubtitleItem& subItem, const std::string& dialogueEvent);

    std::function<void()> m_OnPlaybackEndedCB;

    std::vector<AVFormatContext*> m_Contexts = {};

    //AVFormatContext* m_FormatContext = nullptr;
    AVFormatContext* m_SubtitleFormatContext = nullptr;
    AVCodecContext* m_AudioCodecContext = nullptr;
    AVCodecContext* m_VideoCodecContext = nullptr;
    AVCodecContext* m_SubtitlesCodecContext = nullptr;
    AVChannelLayout m_ChannelLayout;
    AVSampleFormat m_SampleFormat = AV_SAMPLE_FMT_FLT;
    int m_AudioStreamIndex = -1;
    int m_VideoStreamIndex = -1;
    std::vector<winrt::MediaPlayer::SubtitleStream> m_SubtitleStreams;
    int m_CurrentSubSteamIndex;
    uint64_t m_CurrentTime; // milliseconds
    SwrContext* m_SwrContext;
    SwsContext* m_SwsContext;

    AVFilterGraph* m_FilterGraph = nullptr;
    AVFilterContext* m_BufferSrcCtx = nullptr;
    AVFilterContext* m_BufferSinkCtx = nullptr;
    winrt::hstring m_FilterDescStr = L"default";

    std::mutex m_VideoM;
    std::mutex m_AudioM;
    std::condition_variable m_VideoCV;
    std::condition_variable m_AudioCV;

    bool m_FileOpened = false;
    bool m_DecodingPaused = false;

    std::thread m_DecodingThread;
    std::mutex m_Mutex;
};

