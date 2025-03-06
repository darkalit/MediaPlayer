#include "pch.h"
#include "FfmpegDecoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

#include "Utils.h"
#include "MediaConfig.h"

using namespace winrt;

FfmpegDecoder::FfmpegDecoder(std::function<void()> onPlaybackEndedCB)
    : m_OnPlaybackEndedCB(onPlaybackEndedCB)
{
    m_FormatContext = avformat_alloc_context();
}

FfmpegDecoder::~FfmpegDecoder()
{
    m_FileOpened = false;

    if (m_DecodingThread.joinable())
    {
        m_DecodingThread.join();
    }

    if (m_FormatContext)
    {
        avformat_close_input(&m_FormatContext);
    }
    avcodec_free_context(&m_AudioCodecContext);
    avcodec_free_context(&m_VideoCodecContext);
    avformat_free_context(m_FormatContext);
}

bool FfmpegDecoder::HasSource()
{
    return m_AudioStreamIndex != -1 || m_VideoStreamIndex != -1;
}

MediaPlayer::MediaMetadata FfmpegDecoder::GetMetadata(hstring const& filepath)
{
    AVFormatContext* formatContext = nullptr;
    int error = avformat_open_input(&formatContext, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetMetadata avformat_open_input");
        return {};
    }
    defer{ avformat_close_input(&formatContext); };

    error = avformat_find_stream_info(formatContext, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetMetadata avformat_find_stream_info");
        return {};
    }

    MediaPlayer::MediaMetadata metadata = {};
    metadata.Path = filepath;
    metadata.Duration = formatContext->duration / AV_TIME_BASE * 1000;
    metadata.Id = Windows::Foundation::GuidHelper::CreateNewGuid();
    metadata.IsSelected = false;
    metadata.AddedAt = clock::now();

    const AVCodec* audioCodec = nullptr;
    int audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);

    AVCodecParameters* codecParams = formatContext->streams[audioIndex]->codecpar;
    metadata.AudioChannelCount = codecParams->ch_layout.nb_channels;
    metadata.AudioBitrate = codecParams->bit_rate;
    metadata.AudioSampleRate = codecParams->sample_rate;
    metadata.AudioSampleSize = av_get_bytes_per_sample(static_cast<AVSampleFormat>(codecParams->format));

    const AVCodec* videoCodec = nullptr;
    int videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    codecParams = formatContext->streams[videoIndex]->codecpar;
    metadata.VideoBitrate = codecParams->bit_rate;
    metadata.VideoWidth = codecParams->width;
    metadata.VideoHeight = codecParams->height;
    metadata.VideoFrameRate = av_q2d(formatContext->streams[videoIndex]->avg_frame_rate);

    AVDictionaryEntry* tag = nullptr;
    if ((tag = av_dict_get(formatContext->metadata, "author", nullptr, 0)))
    {
        metadata.Author = to_hstring(tag->value);
    }
    if ((tag = av_dict_get(formatContext->metadata, "title", nullptr, 0)))
    {
        metadata.Title = to_hstring(tag->value);
    }
    if ((tag = av_dict_get(formatContext->metadata, "album", nullptr, 0)))
    {
        metadata.AlbumTitle = to_hstring(tag->value);
    }
    
    return metadata;
}

void FfmpegDecoder::OpenFile(hstring const& filepath)
{
    if (m_FileOpened && m_FormatContext)
    {
        m_FileOpened = false;
    }

    if (m_DecodingThread.joinable())
    {
        m_DecodingThread.join();
    }

    if (m_FormatContext)
    {
        avformat_close_input(&m_FormatContext);
    }

    m_SubtitleStreams.clear();
    m_SubtitleStreams.resize(0);

    int error = avformat_open_input(&m_FormatContext, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avformat_open_input");
        return;
    }
    defer{ m_FileOpened = true; };

    error = avformat_find_stream_info(m_FormatContext, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avformat_find_stream_info");
        return;
    }

    if (m_SubtitleFormatContext)
    {
        avformat_close_input(&m_SubtitleFormatContext);
        m_SubtitleFormatContext = nullptr;
    }
    
    GetSubtitles(m_FormatContext);

    const AVCodec* audioCodec = nullptr;
    m_AudioStreamIndex = av_find_best_stream(m_FormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);
    if (m_AudioStreamIndex < 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile Audio av_find_best_stream");
        return;
    }

    m_AudioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!m_AudioCodecContext)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile Audio avcodec_alloc_context3");
        return;
    }

    error = avcodec_parameters_to_context(m_AudioCodecContext, m_FormatContext->streams[m_AudioStreamIndex]->codecpar);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile Audio avcodec_parameters_to_context");
        return;
    }

    error = avcodec_open2(m_AudioCodecContext, audioCodec, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile Audio avcodec_open2");
        return;
    }

    av_channel_layout_default(&m_ChannelLayout, MediaConfig::AudioChannels);

    error = swr_alloc_set_opts2(
        &m_SwrContext,
        // Output
        &m_ChannelLayout,
        m_SampleFormat,
        MediaConfig::AudioSampleRate,
        // Input
        &m_AudioCodecContext->ch_layout,
        m_AudioCodecContext->sample_fmt,
        m_AudioCodecContext->sample_rate,
        0, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile Audio swr_alloc_set_opts2");
        return;
    }

    error = swr_init(m_SwrContext);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile Audio swr_init");
        return;
    }

    const AVCodec* videoCodec = nullptr;
    m_VideoStreamIndex = av_find_best_stream(m_FormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    if (m_VideoStreamIndex >= 0)
    {
        m_VideoCodecContext = avcodec_alloc_context3(videoCodec);
        if (!m_VideoCodecContext)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Video avcodec_alloc_context3");
            return;
        }

        error = avcodec_parameters_to_context(m_VideoCodecContext, m_FormatContext->streams[m_VideoStreamIndex]->codecpar);
        if (error != 0)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Video avcodec_parameters_to_context");
            return;
        }

        error = avcodec_open2(m_VideoCodecContext, videoCodec, nullptr);
        if (error != 0)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Video avcodec_open2");
            return;
        }

        m_SwsContext = sws_getContext(
            m_VideoCodecContext->width,
            m_VideoCodecContext->height,
            m_VideoCodecContext->pix_fmt,
            m_VideoCodecContext->width,
            m_VideoCodecContext->height,
            AV_PIX_FMT_RGBA, SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);
    }

    //error = av_seek_frame(m_FormatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
    //if (error != 0)
    //{
    //    OutputDebugString(L"FfmpegDecoder::OpenFile av_seek_frame");
    //    return;
    //}
}

void FfmpegDecoder::OpenSubtitle(winrt::hstring const& filepath)
{
    m_CurrentSubSteamIndex = -1;
    if (m_SubtitleFormatContext)
    {
        avformat_close_input(&m_SubtitleFormatContext);
        m_SubtitleFormatContext = nullptr;
    }
    int error = avformat_open_input(&m_SubtitleFormatContext, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle avformat_open_input");
        return;
    }

    error = avformat_find_stream_info(m_SubtitleFormatContext, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle avformat_find_stream_info");
        return;
    }

    m_CurrentSubSteamIndex = av_find_best_stream(m_SubtitleFormatContext, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);
    if (m_CurrentSubSteamIndex < 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles av_find_best_stream");
        return;
    }

    const AVCodec* subtitlesCodec = avcodec_find_decoder(m_SubtitleFormatContext->streams[m_CurrentSubSteamIndex]->codecpar->codec_id);;

    m_SubtitlesCodecContext = avcodec_alloc_context3(subtitlesCodec);
    if (!m_SubtitlesCodecContext)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_alloc_context3");
        return;
    }

    error = avcodec_parameters_to_context(m_SubtitlesCodecContext, m_SubtitleFormatContext->streams[m_CurrentSubSteamIndex]->codecpar);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_parameters_to_context");
        return;
    }

    error = avcodec_open2(m_SubtitlesCodecContext, subtitlesCodec, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_open2");
        return;
    }
}

void FfmpegDecoder::OpenSubtitle(unsigned int subtitleIndex)
{
    if (subtitleIndex == -1)
    {
        m_CurrentSubSteamIndex = subtitleIndex;
        return;
    }

    m_CurrentSubSteamIndex = -1;
    if (m_SubtitleFormatContext)
    {
        avformat_close_input(&m_SubtitleFormatContext);
        m_SubtitleFormatContext = nullptr;
    }
    const AVCodec* subtitlesCodec = avcodec_find_decoder(m_FormatContext->streams[subtitleIndex]->codecpar->codec_id);;

    m_SubtitlesCodecContext = avcodec_alloc_context3(subtitlesCodec);
    if (!m_SubtitlesCodecContext)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_alloc_context3");
        return;
    }

    int error = avcodec_parameters_to_context(m_SubtitlesCodecContext, m_FormatContext->streams[subtitleIndex]->codecpar);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_parameters_to_context");
        return;
    }

    error = avcodec_open2(m_SubtitlesCodecContext, subtitlesCodec, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_open2");
        return;
    }

    m_CurrentSubSteamIndex = subtitleIndex;
}

void FfmpegDecoder::SetupDecoding(SharedQueue<VideoFrame>& frames, SharedQueue<SubtitleItem>& subs, SharedQueue<AudioSample>& audioSamples)
{
    if (m_DecodingThread.joinable())
    {
        m_DecodingThread.join();
    }

    m_DecodingThread = std::thread([&]()
    {
        OutputDebugString(L"New decode thread\n");

        auto frame = av_frame_alloc();
        defer{ av_frame_free(&frame); };

        auto packet = av_packet_alloc();
        defer{ av_packet_free(&packet); };
        int error = 0;

        while (m_FileOpened)
        {
            if (m_DecodingPaused || (audioSamples.Size() > 18))
            {
                continue;
            }
            std::lock_guard lock(m_Mutex);

            int ret = av_read_frame(m_FormatContext, packet);
            if (ret == AVERROR(EAGAIN))
            {
                continue;
            }
            if (ret == AVERROR_EOF)
            {
                m_OnPlaybackEndedCB();
            }
            if (ret < 0)
            {
                break;
            }
            defer{ av_packet_unref(packet); };

            if (packet->stream_index == m_VideoStreamIndex)
            {
                error = avcodec_send_packet(m_VideoCodecContext, packet);
                if (error != 0)
                {
                    OutputDebugString(L"FfmpegDecoder::GetNextFrame avcodec_send_packet");
                    continue;
                }

                while (avcodec_receive_frame(m_VideoCodecContext, frame) == 0)
                {
                    uint8_t* data[4];
                    int linesize[4];

                    error = av_image_alloc(
                        data,
                        linesize,
                        m_VideoCodecContext->width,
                        m_VideoCodecContext->height,
                        AV_PIX_FMT_RGBA,
                        1);
                    if (error < 0)
                    {
                        OutputDebugString(L"FfmpegDecoder::GetNextFrame av_image_alloc");
                        continue;
                    }
                    defer{ av_freep(&data[0]); };

                    sws_scale(
                        m_SwsContext,
                        frame->data,
                        frame->linesize,
                        0,
                        m_VideoCodecContext->height,
                        data,
                        linesize);

                    VideoFrame outFrame;
                    outFrame.Width = m_VideoCodecContext->width;
                    outFrame.Height = m_VideoCodecContext->height;
                    outFrame.Buffer.assign(data[0], data[0] + linesize[0] * outFrame.Height);
                    outFrame.RowPitch = linesize[0];
                    outFrame.StartTime = static_cast<double>(frame->best_effort_timestamp) * av_q2d(m_FormatContext->streams[m_VideoStreamIndex]->time_base);
                    frames.Push(outFrame);
                }
            }
            else if (packet->stream_index == m_AudioStreamIndex)
            {
                error = avcodec_send_packet(m_AudioCodecContext, packet);
                if (error != 0)
                {
                    continue;
                }

                AudioSample sample;

                while(avcodec_receive_frame(m_AudioCodecContext, frame) == 0)
                {
                    int outSamples = swr_get_out_samples(m_SwrContext, frame->nb_samples);
                    uint8_t* buffer = nullptr;
                    av_samples_alloc(&buffer, nullptr, m_ChannelLayout.nb_channels, outSamples, m_SampleFormat, 1);
                    outSamples = swr_convert(
                        m_SwrContext,
                        &buffer,
                        outSamples,
                        frame->data,
                        frame->nb_samples);
                    int dataSize = av_samples_get_buffer_size(nullptr, m_ChannelLayout.nb_channels, outSamples, m_SampleFormat, 0);
                    //int dataSize = outSamples * m_ChannelLayout.nb_channels * 2;
                    float* floatBuffer = reinterpret_cast<float*>(buffer);

                    sample.Buffer.insert(sample.Buffer.end(), floatBuffer, floatBuffer + dataSize / sizeof(float));

                    if (buffer)
                    {
                        av_free(buffer);
                    }
                }
                double timeBase = av_q2d(m_FormatContext->streams[m_AudioStreamIndex]->time_base);
                sample.Duration = packet->duration * timeBase;
                sample.StartTime = packet->pts * timeBase;
                m_CurrentTime = static_cast<uint64_t>(packet->pts * timeBase * 1000.0);
                audioSamples.Push(sample);
            }
            else if (packet->stream_index == m_CurrentSubSteamIndex)
            {
                error = PushSubtitle(subs, m_FormatContext, packet);
                if (error < 0)
                {
                    OutputDebugString(L"FfmpegDecoder::SetupDecoding Subtitles PushSubtitle");
                    continue;
                }
            }

            if (m_SubtitleFormatContext)
            {
                av_packet_unref(packet);
                if (av_read_frame(m_SubtitleFormatContext, packet) < 0) continue;

                if (packet->stream_index == m_CurrentSubSteamIndex)
                {
                    error = PushSubtitle(subs, m_SubtitleFormatContext, packet);
                    if (error < 0)
                    {
                        OutputDebugString(L"FfmpegDecoder::SetupDecoding Subtitles PushSubtitle");
                        continue;
                    }
                }
            }
        }
    });
}

void FfmpegDecoder::PauseDecoding(bool pause)
{
    m_DecodingPaused = pause;
}

uint64_t FfmpegDecoder::GetPosition()
{
    return m_CurrentTime;
}

std::vector<MediaPlayer::SubtitleStream>& FfmpegDecoder::GetSubtitleStreams()
{
    return m_SubtitleStreams;
}

void FfmpegDecoder::Seek(uint64_t time)
{
    std::lock_guard lock(m_Mutex);

    std::vector streamIndexes = { m_VideoStreamIndex, m_AudioStreamIndex };
    std::vector codecContexes = { m_VideoCodecContext, m_AudioCodecContext };
    for (auto index : streamIndexes)
    {
        const int64_t seekTarget = av_rescale_q(
            time * 1000,
            AV_TIME_BASE_Q,
            m_FormatContext->streams[index]->time_base
        );

        av_seek_frame(m_FormatContext, m_VideoStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD);
    }

    for (auto codecContext : codecContexes)
    {
        if (codecContext)
        {
            avcodec_flush_buffers(codecContext);
        }
    }
}

void FfmpegDecoder::RecordSegment(winrt::hstring const& filepath, uint64_t start, uint64_t end)
{
    AVFormatContext* inContext = nullptr;
    int error = avformat_open_input(&inContext, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::RecordSegment avformat_open_input");
        return;
    }
    defer{ avformat_close_input(&inContext); };

    error = avformat_find_stream_info(inContext, nullptr);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::RecordSegment avformat_find_stream_info");
        return;
    }

    int videoStreamId = av_find_best_stream(inContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int audioStreamId = av_find_best_stream(inContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    int64_t startPts = start / 1000.0f * AV_TIME_BASE;
    error = av_seek_frame(inContext, -1, startPts, AVSEEK_FLAG_BACKWARD);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::RecordSegment av_seek_frame");
        return;
    }
    avformat_flush(inContext);

    std::string outFilename = to_string(filepath) + "_" + std::to_string(start / 1000.0f) + "-" + std::to_string(end / 1000.0f) + ".mp4";
    AVFormatContext* outContext = nullptr;
    error = avformat_alloc_output_context2(&outContext, nullptr, nullptr, outFilename.c_str());
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::RecordSegment avformat_alloc_output_context2");
        return;
    }
    defer{ avformat_free_context(outContext); };

    std::vector<int> streamMapping(inContext->nb_streams);

    for (int i = 0, streamIndex = 0; i < inContext->nb_streams; ++i)
    {
        AVStream* inStream = inContext->streams[i];
        if (inStream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            inStream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
        {
            streamMapping[i] = -1;
            continue;
        }

        streamMapping[i] = streamIndex++;

        AVStream* outStream = avformat_new_stream(outContext, nullptr);
        if (!outStream)
        {
            OutputDebugString(L"FfmpegDecoder::RecordSegment avformat_new_stream");
            return;
        }

        error = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        if (error < 0)
        {
            OutputDebugString(L"FfmpegDecoder::RecordSegment avcodec_parameters_copy");
            return;
        }

        outStream->time_base = inStream->time_base;
    }

    if (!(outContext->oformat->flags & AVFMT_NOFILE))
    {
        error = avio_open(&outContext->pb, outFilename.c_str(), AVIO_FLAG_WRITE);
        if (error < 0)
        {
            OutputDebugString(L"FfmpegDecoder::RecordSegment avio_open");
            return;
        }
    }
    defer{
        if (!(outContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&outContext->pb);
    };

    error = avformat_write_header(outContext, nullptr);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::RecordSegment avformat_write_header");
        return;
    }
    defer{ av_write_trailer(outContext); };

    int64_t endPts = end / 1000.0f * AV_TIME_BASE;
    AVPacket packet;
    std::unordered_map<int, int64_t> firstPtsMap;
    while (av_read_frame(inContext, &packet) >= 0)
    {
        defer{ av_packet_unref(&packet); };
        int inStreamIndex = packet.stream_index;
        if (inStreamIndex < 0 || inStreamIndex >= streamMapping.size() || streamMapping[inStreamIndex] < 0)
        {
            continue;
        }

        AVStream* inStream = inContext->streams[inStreamIndex];
        AVStream* outStream = outContext->streams[streamMapping[inStreamIndex]];

        int64_t packetTime = av_rescale_q(packet.pts, inStream->time_base, AVRational{ 1, AV_TIME_BASE });
        if (packetTime < startPts || packetTime > endPts)
        {
            continue;
        }

        if (firstPtsMap.find(inStreamIndex) == firstPtsMap.end())
        {
            firstPtsMap[inStreamIndex] = packet.pts;
        }
        int64_t offset = firstPtsMap[inStreamIndex];

        packet.pts -= offset;
        packet.dts -= offset;

        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;
        packet.stream_index = streamMapping[inStreamIndex];

        if (av_interleaved_write_frame(outContext, &packet) < 0)
        {
            OutputDebugString(L"FfmpegDecoder::RecordSegment av_interleaved_write_frame");
            break;
        }
    }
}

void FfmpegDecoder::GetSubtitles(AVFormatContext* formatContext)
{
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
    {
        AVCodecParameters* codecParams = formatContext->streams[i]->codecpar;
        if (codecParams->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            AVStream* subtitleStream = formatContext->streams[i];

            MediaPlayer::SubtitleStream stream = {};
            stream.Index = i;

            auto titleTag = av_dict_get(subtitleStream->metadata, "title", nullptr, 0);
            if (titleTag) stream.Title = to_hstring(titleTag->value);

            auto langTag = av_dict_get(subtitleStream->metadata, "language", nullptr, 0);
            if (langTag) stream.Language = to_hstring(langTag->value);

            m_SubtitleStreams.push_back(stream);
        }
    }
}

int FfmpegDecoder::PushSubtitle(SharedQueue<SubtitleItem>& subs, AVFormatContext* formatContext, AVPacket* packet)
{
    int gotSub = 0;
    AVSubtitle subtitle;
    int error = avcodec_decode_subtitle2(m_SubtitlesCodecContext, &subtitle, &gotSub, packet);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::PushSubtitle Subtitles avcodec_decode_subtitle2");
        return error;
    }
    defer{ avsubtitle_free(&subtitle); };

    if (gotSub)
    {
        SubtitleItem subItem = {};
        for (unsigned int i = 0; i < subtitle.num_rects; ++i)
        {
            AVSubtitleRect* rect = subtitle.rects[i];
            if (rect->text)
            {
                subItem.Text = to_hstring(rect->text);
            }
            else if (rect->ass)
            {
                ParseAssDialogue(subItem, rect->ass);
            }
        }

        if (subItem.StartTime == 0 || subItem.EndTime == 0)
        {
            double subTimeBase = av_q2d(formatContext->streams[m_CurrentSubSteamIndex]->time_base);
            subItem.StartTime = packet->pts * subTimeBase;

            if (subtitle.end_display_time > 0)
            {
                subItem.EndTime = subItem.StartTime + (subtitle.end_display_time / 1000.0);
            }
            else
            {
                subItem.EndTime = subItem.StartTime + packet->duration * subTimeBase;
            }
        }

        subs.Push(subItem);
    }
}

void FfmpegDecoder::ParseAssDialogue(SubtitleItem& subItem, const std::string& dialogueEvent)
{
    std::istringstream stream(dialogueEvent);
    std::string token;

    // Proper dialogue event format:
    // Marked, Layer*, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
    // But got:
    // Layer, ???, Style, Name, MarginL, MarginR, MarginV, Effect, Text

    // Layer
    std::getline(stream, token, ',');

    // ???
    std::getline(stream, token, ',');

    // Style
    std::getline(stream, token, ',');

    // Name
    std::getline(stream, token, ',');
    subItem.Name = to_hstring(token);

    // MarginL
    std::getline(stream, token, ',');

    // MarginR
    std::getline(stream, token, ',');

    // MarginV
    std::getline(stream, token, ',');

    // Effect
    std::getline(stream, token, ',');

    // Text
    std::getline(stream, token);
    subItem.Text = to_hstring(token);
}
