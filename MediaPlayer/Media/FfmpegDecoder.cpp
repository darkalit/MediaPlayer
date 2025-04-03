#include "pch.h"
#include "FfmpegDecoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/channel_layout.h"
}

#include "Utils.h"
#include "MediaConfig.h"

using namespace winrt;

FfmpegDecoder::FfmpegDecoder(std::function<void()> onPlaybackEndedCB)
    : m_OnPlaybackEndedCB(onPlaybackEndedCB)
{
    //m_FormatContext = avformat_alloc_context();
}

FfmpegDecoder::~FfmpegDecoder()
{
    Free();
}

bool FfmpegDecoder::HasSource()
{
    return m_AudioStreamIndex != -1 || m_VideoStreamIndex != -1;
}

MediaPlayer::MediaMetadata FfmpegDecoder::GetMetadata(hstring const& filepath, MediaType mediaType)
{
    av_log_set_level(AV_LOG_DEBUG);
    AVFormatContext* formatContext = nullptr;
    AVDictionary* options = nullptr;
    av_dict_set(&options, "http_follow", "1", 0);
    av_dict_set(&options, "user_agent", "Mozilla/5.0", 0);

    int error = avformat_open_input(&formatContext, to_string(filepath).c_str(), nullptr, &options);
    if (error != 0)
    {
        std::string errstr(AV_ERROR_MAX_STRING_SIZE, '\0');
        av_make_error_string(errstr.data(), errstr.size(), error);
        OutputDebugString(L"FfmpegDecoder::GetMetadata avformat_open_input: ");
        OutputDebugString(to_hstring(errstr).c_str());
        OutputDebugString(L"\n");
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

    AVCodecParameters* codecParams = nullptr;

    if (mediaType & MediaType::AUDIO)
    {
        const AVCodec* audioCodec = nullptr;
        int audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);

        codecParams = formatContext->streams[audioIndex]->codecpar;
        metadata.AudioChannelCount = codecParams->ch_layout.nb_channels;
        metadata.AudioBitrate = codecParams->bit_rate;
        metadata.AudioSampleRate = codecParams->sample_rate;
        metadata.AudioSampleSize = av_get_bytes_per_sample(static_cast<AVSampleFormat>(codecParams->format));
    }

    if (mediaType & MediaType::VIDEO)
    {
        const AVCodec* videoCodec = nullptr;
        int videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

        codecParams = formatContext->streams[videoIndex]->codecpar;
        metadata.VideoBitrate = codecParams->bit_rate;
        metadata.VideoWidth = codecParams->width;
        metadata.VideoHeight = codecParams->height;
        metadata.VideoFrameRate = av_q2d(formatContext->streams[videoIndex]->avg_frame_rate);
    }

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

void FfmpegDecoder::OpenByStreams(winrt::hstring const& video, winrt::hstring const& audio)
{
    Free();

    AVFormatContext* videoContext = nullptr;
    AVFormatContext* audioContext = nullptr;

    int error = avformat_open_input(&videoContext, to_string(video).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
        auto errStr = av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
        OutputDebugString(L"FfmpegDecoder::OpenByStreams VIDEO avformat_open_input\n");
        OutputDebugString(to_hstring(errStr).c_str());
        return;
    }

    error = avformat_find_stream_info(videoContext, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams VIDEO avformat_find_stream_info");
        return;
    }

    error = avformat_open_input(&audioContext, to_string(audio).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO avformat_open_input");
        return;
    }

    error = avformat_find_stream_info(audioContext, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO avformat_find_stream_info");
        return;
    }

    defer{ m_FileOpened = true; };

    const AVCodec* audioCodec = nullptr;
    m_AudioStreamIndex = av_find_best_stream(audioContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);
    if (m_AudioStreamIndex < 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO av_find_best_stream");
        return;
    }

    m_AudioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!m_AudioCodecContext)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO avcodec_alloc_context3");
        return;
    }

    error = avcodec_parameters_to_context(m_AudioCodecContext, audioContext->streams[m_AudioStreamIndex]->codecpar);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO avcodec_parameters_to_context");
        return;
    }

    error = avcodec_open2(m_AudioCodecContext, audioCodec, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO avcodec_open2");
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
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO swr_alloc_set_opts2");
        return;
    }

    error = swr_init(m_SwrContext);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenByStreams AUDIO swr_init");
        return;
    }

    const AVCodec* videoCodec = nullptr;
    m_VideoStreamIndex = av_find_best_stream(videoContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    if (m_VideoStreamIndex >= 0)
    {
        m_VideoCodecContext = avcodec_alloc_context3(videoCodec);
        if (!m_VideoCodecContext)
        {
            OutputDebugString(L"FfmpegDecoder::OpenByStreams VIDEO avcodec_alloc_context3");
            return;
        }

        error = avcodec_parameters_to_context(m_VideoCodecContext, videoContext->streams[m_VideoStreamIndex]->codecpar);
        if (error != 0)
        {
            OutputDebugString(L"FfmpegDecoder::OpenByStreams VIDEO avcodec_parameters_to_context");
            return;
        }

        error = avcodec_open2(m_VideoCodecContext, videoCodec, nullptr);
        if (error != 0)
        {
            OutputDebugString(L"FfmpegDecoder::OpenByStreams VIDEO avcodec_open2");
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

    m_Contexts.push_back(audioContext);
    m_Contexts.push_back(videoContext);
}

void FfmpegDecoder::OpenFile(hstring const& filepath)
{
    Free();

    m_SubtitleStreams.clear();
    m_SubtitleStreams.resize(0);

    AVFormatContext* context = avformat_alloc_context();

    int error = avformat_open_input(&context, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avformat_open_input");
        return;
    }
    defer{ m_FileOpened = true; };

    error = avformat_find_stream_info(context, nullptr);
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
    
    GetSubtitles(context);

    const AVCodec* audioCodec = nullptr;
    m_AudioStreamIndex = av_find_best_stream(context, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);
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

    error = avcodec_parameters_to_context(m_AudioCodecContext, context->streams[m_AudioStreamIndex]->codecpar);
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
    m_VideoStreamIndex = av_find_best_stream(context, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    if (m_VideoStreamIndex >= 0)
    {
        m_VideoCodecContext = avcodec_alloc_context3(videoCodec);
        if (!m_VideoCodecContext)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Video avcodec_alloc_context3");
            return;
        }

        error = avcodec_parameters_to_context(m_VideoCodecContext, context->streams[m_VideoStreamIndex]->codecpar);
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

    m_Contexts.push_back(context);

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

    AVFormatContext* context = nullptr;
    for (auto& ctx : m_Contexts)
    {
        if (subtitleIndex >= ctx->nb_streams) continue;

        if (ctx->streams[subtitleIndex]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            context = ctx;
            break;
        }
    }

    m_CurrentSubSteamIndex = -1;
    if (m_SubtitleFormatContext)
    {
        avformat_close_input(&m_SubtitleFormatContext);
        m_SubtitleFormatContext = nullptr;
    }
    const AVCodec* subtitlesCodec = avcodec_find_decoder(context->streams[subtitleIndex]->codecpar->codec_id);;

    m_SubtitlesCodecContext = avcodec_alloc_context3(subtitlesCodec);
    if (!m_SubtitlesCodecContext)
    {
        OutputDebugString(L"FfmpegDecoder::OpenSubtitle Subtitles avcodec_alloc_context3");
        return;
    }

    int error = avcodec_parameters_to_context(m_SubtitlesCodecContext, context->streams[subtitleIndex]->codecpar);
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
        int error = 0;

        auto frame = av_frame_alloc();
        defer{ av_frame_free(&frame); };

        auto filtFrame = av_frame_alloc();
        defer{ av_frame_free(&filtFrame); };

        auto packet = av_packet_alloc();
        defer{ av_packet_free(&packet); };

        while (m_FileOpened)
        {
            if (m_DecodingPaused || frames.Size() > 12)
            {
                continue;
            }
            std::lock_guard lock(m_Mutex);

            for (auto& context : m_Contexts)
            {

                int ret = av_read_frame(context, packet);
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

                if (packet->stream_index == m_VideoStreamIndex && context->streams[m_VideoStreamIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
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
                        outFrame.StartTime = static_cast<double>(frame->best_effort_timestamp) * av_q2d(context->streams[m_VideoStreamIndex]->time_base);
                        frames.Push(outFrame);
                    }
                }
                else if (packet->stream_index == m_AudioStreamIndex && context->streams[m_AudioStreamIndex]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    error = avcodec_send_packet(m_AudioCodecContext, packet);
                    if (error != 0)
                    {
                        continue;
                    }

                    AudioSample sample;

                    while (avcodec_receive_frame(m_AudioCodecContext, frame) == 0)
                    {
                        int outSamples = swr_get_out_samples(m_SwrContext, frame->nb_samples);
                        uint8_t* buffer = nullptr;
                        av_samples_alloc(&buffer, nullptr, m_ChannelLayout.nb_channels, outSamples, m_SampleFormat, 1);
                        defer {
                            if (buffer)
                            {
                                av_free(buffer);
                            }
                        };

                        outSamples = swr_convert(
                            m_SwrContext,
                            &buffer,
                            outSamples,
                            frame->data,
                            frame->nb_samples);
                        int dataSize = av_samples_get_buffer_size(nullptr, m_ChannelLayout.nb_channels, outSamples, m_SampleFormat, 0);

                        if (m_FilterDescStr == L"default")
                        {
                            float* floatBuffer = reinterpret_cast<float*>(buffer);
                            sample.Buffer.insert(sample.Buffer.end(), floatBuffer, floatBuffer + dataSize / sizeof(float));
                        }
                        else
                        {
                            AVFrame* convFrame = av_frame_alloc();
                            defer { av_frame_free(&convFrame); };
                            convFrame->nb_samples = outSamples;
                            convFrame->ch_layout = m_ChannelLayout;
                            convFrame->format = m_SampleFormat;
                            convFrame->sample_rate = MediaConfig::AudioSampleRate;

                            av_samples_fill_arrays(convFrame->data, convFrame->linesize, buffer, m_ChannelLayout.nb_channels, outSamples, m_SampleFormat, 0);

                            error = av_buffersrc_add_frame_flags(m_BufferSrcCtx, convFrame, AV_BUFFERSRC_FLAG_KEEP_REF);
                            if (error < 0)
                            {
                                char err[AV_ERROR_MAX_STRING_SIZE] = {};
                                auto errStr = av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, error);
                                OutputDebugString(to_hstring(errStr).c_str());
                                continue;
                            }

                            error = av_buffersink_get_frame(m_BufferSinkCtx, filtFrame);
                            if (error < 0)
                            {
                                continue;
                            }

                            int filtOutSamples = swr_get_out_samples(m_SwrContext, filtFrame->nb_samples);
                            uint8_t* filtBuffer = nullptr;
                            av_samples_alloc(&filtBuffer, nullptr, m_ChannelLayout.nb_channels, filtOutSamples, m_SampleFormat, 1);
                            defer {
                                if (filtBuffer)
                                {
                                    av_free(filtBuffer);
                                }
                            };

                            filtOutSamples = swr_convert(
                                m_SwrContext,
                                &filtBuffer,
                                filtOutSamples,
                                filtFrame->data,
                                filtFrame->nb_samples);
                            int filtDataSize = av_samples_get_buffer_size(nullptr, m_ChannelLayout.nb_channels, filtOutSamples, m_SampleFormat, 0);

                            float* floatBuffer = reinterpret_cast<float*>(filtBuffer);

                            sample.Buffer.insert(sample.Buffer.end(), floatBuffer, floatBuffer + filtDataSize / sizeof(float));
                        }
                    }

                    double timeBase = av_q2d(context->streams[m_AudioStreamIndex]->time_base);
                    sample.Duration = packet->duration * timeBase;
                    sample.StartTime = packet->pts * timeBase;
                    m_CurrentTime = static_cast<uint64_t>(packet->pts * timeBase * 1000.0);
                    audioSamples.Push(sample);
                }
                else if (packet->stream_index == m_CurrentSubSteamIndex && context->streams[m_CurrentSubSteamIndex]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
                {
                    error = PushSubtitle(subs, context, packet);
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
        }

        m_VideoCV.notify_all();
        m_AudioCV.notify_all();
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

    std::vector streamTypes = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    std::vector codecContexes = { m_VideoCodecContext, m_AudioCodecContext };
    for (auto& context : m_Contexts)
    {
        for (auto type : streamTypes)
        {
            const AVCodec* codec = nullptr;
            int index = av_find_best_stream(context, type, -1, -1, &codec, 0);
            if (index < 0)
            {
                continue;
            }

            const int64_t seekTarget = av_rescale_q(
                time * 1000,
                AV_TIME_BASE_Q,
                context->streams[index]->time_base
            );

            av_seek_frame(context, index, seekTarget, AVSEEK_FLAG_BACKWARD);
        }
    }

    for (auto codecContext : codecContexes)
    {
        if (codecContext)
        {
            avcodec_flush_buffers(codecContext);
        }
    }
}

void FfmpegDecoder::SetAudioFilter(winrt::hstring const& filterStr)
{
    std::lock_guard lock(m_Mutex);

    m_FilterDescStr = filterStr;
    FreeFilters();
    InitAudioFilter(m_FilterDescStr);
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

VideoFrame FfmpegDecoder::GetFrame(winrt::hstring const& filepath, uint64_t pos, int height)
{
    AVFormatContext* context = nullptr;
    int error = avformat_open_input(&context, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame avformat_open_input");
        return {};
    }
    defer{ avformat_close_input(&context); };

    error = avformat_find_stream_info(context, nullptr);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame avformat_find_stream_info");
        return {};
    }

    int videoStreamId = av_find_best_stream(context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    int64_t startPts = pos / 1000.0f * AV_TIME_BASE;
    error = av_seek_frame(context, -1, startPts, AVSEEK_FLAG_BACKWARD);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame av_seek_frame");
        return {};
    }
    avformat_flush(context);

    auto codecPar = context->streams[videoStreamId]->codecpar;
    auto codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame avcodec_find_decoder");
        return {};
    }

    auto codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame avcodec_alloc_context3");
        return {};
    }
    defer{ avcodec_free_context(&codecContext); };

    error = avcodec_parameters_to_context(codecContext, codecPar);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame avcodec_parameters_to_context");
        return {};
    }

    error = avcodec_open2(codecContext, codec, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame avcodec_open2");
        return {};
    }

    auto frame = av_frame_alloc();
    defer{ av_frame_free(&frame); };

    auto packet = av_packet_alloc();
    defer{ av_packet_free(&packet); };

    bool gotFrame = false;
    while (av_read_frame(context, packet) >= 0)
    {
        defer{ av_packet_unref(packet); };
        if (packet->stream_index != videoStreamId) continue;

        error = avcodec_send_packet(codecContext, packet);
        if (error == AVERROR(EAGAIN))
        {
            OutputDebugString(L"AVERROR(EAGAIN)");
        }
        if (error == AVERROR_EOF)
        {
            OutputDebugString(L"AVERROR_EOF");
        }
        if (error == AVERROR(EINVAL))
        {
            OutputDebugString(L"AVERROR(EINVAL)");
        }
        if (error < 0) continue;

        error = avcodec_receive_frame(codecContext, frame);
        if (error < 0) continue;

        gotFrame = true;
        break;
    }

    if (!gotFrame)
    {
        return {};
    }

    int targetHeight = max(height, 0);
    int targetWidth = codecContext->width * targetHeight / codecContext->height;

    auto swsContext = sws_getContext(
        codecContext->width,
        codecContext->height,
        codecContext->pix_fmt,
        targetWidth,
        targetHeight,
        AV_PIX_FMT_BGRA, SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);

    if (!swsContext)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame sws_getContext");
        return {};
    }

    auto dstFrame = av_frame_alloc();
    defer{ av_frame_free(&dstFrame); };

    uint8_t* data[4];
    int linesize[4];

    error = av_image_alloc(
        data,
        linesize,
        targetWidth,
        targetHeight,
        AV_PIX_FMT_BGRA,
        1);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetFrame av_image_alloc");
        return {};
    }
    defer{ av_freep(&data[0]); };

    sws_scale(
        swsContext,
        frame->data,
        frame->linesize,
        0,
        codecContext->height,
        data,
        linesize);

    VideoFrame outFrame;
    outFrame.Width = targetWidth;
    outFrame.Height = targetHeight;
    outFrame.Buffer.assign(data[0], data[0] + linesize[0] * outFrame.Height);
    outFrame.RowPitch = linesize[0];
    outFrame.StartTime = 0;

    return outFrame;
}

void FfmpegDecoder::Free()
{
    m_FileOpened = false;

    if (m_DecodingThread.joinable())
    {
        m_DecodingThread.join();
    }

    //if (m_FormatContext)
    //{
    //    avformat_close_input(&m_FormatContext);
    //    avformat_free_context(m_FormatContext);
    //}

    for (auto& context : m_Contexts)
    {
        avformat_close_input(&context);
        avformat_free_context(context);
    }
    m_Contexts.clear();

    if (m_AudioCodecContext)
    {
        avcodec_free_context(&m_AudioCodecContext);
    }

    if (m_VideoCodecContext)
    {
        avcodec_free_context(&m_VideoCodecContext);
    }

    if (m_SubtitlesCodecContext)
    {
        avcodec_free_context(&m_SubtitlesCodecContext);
    }

    FreeFilters();
}

void FfmpegDecoder::FreeFilters()
{
    if (m_FilterGraph)
    {
        avfilter_graph_free(&m_FilterGraph);    
    }
}

void FfmpegDecoder::InitAudioFilter(winrt::hstring const& filterStr)
{
    if (filterStr == L"default") return;

    int error = 0;
    m_FilterGraph = avfilter_graph_alloc();
    if (!m_FilterGraph)
    {
        OutputDebugString(L"FfmpegDecoder::SetupDecoding avfilter_graph_alloc");
        return;
    }

    const AVFilter* bufferSrc = avfilter_get_by_name("buffersrc");
    const AVFilter* bufferSink = avfilter_get_by_name("buffersink");
    m_BufferSrcCtx = nullptr;
    m_BufferSinkCtx = nullptr;

    AVFilterInOut* outF = avfilter_inout_alloc();
    defer{ avfilter_inout_free(&outF); };

    AVFilterInOut* inF = avfilter_inout_alloc();
    defer{ avfilter_inout_free(&inF); };

    char args[512];
    snprintf(args, sizeof(args),
        "time_base=1/%d:sample_rate=%d:sample_fmt=flt:channel_layout=stereo",
        MediaConfig::AudioSampleRate, MediaConfig::AudioSampleRate);
    // m_SampleFormat
    // MediaConfig::AudioChannels
    error = avfilter_graph_create_filter(&m_BufferSrcCtx, bufferSrc, "in", args, nullptr, m_FilterGraph);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::SetupDecoding avfilter_graph_create_filter SRC");
        return;
    }

    error = avfilter_graph_create_filter(&m_BufferSinkCtx, bufferSink, "out", nullptr, nullptr, m_FilterGraph);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::SetupDecoding avfilter_graph_create_filter SINK");
        return;
    }

    outF->name = av_strdup("in");
    outF->filter_ctx = m_BufferSrcCtx;
    outF->pad_idx = 0;
    outF->next = nullptr;

    inF->name = av_strdup("out");
    inF->filter_ctx = m_BufferSinkCtx;
    inF->pad_idx = 0;
    inF->next = nullptr;

    //std::string filterDesc = "aecho=0.8:0.88:60:0.4";
    error = avfilter_graph_parse_ptr(m_FilterGraph, to_string(filterStr).c_str(), &inF, &outF, nullptr);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::SetupDecoding avfilter_graph_parse_ptr");
        return;
    }

    error = avfilter_graph_config(m_FilterGraph, nullptr);
    if (error < 0)
    {
        OutputDebugString(L"FfmpegDecoder::SetupDecoding avfilter_graph_config");
        return;
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
