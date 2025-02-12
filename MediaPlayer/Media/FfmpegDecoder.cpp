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

using namespace winrt;

FfmpegDecoder::FfmpegDecoder()
{
    m_FormatContext = avformat_alloc_context();
}

FfmpegDecoder::~FfmpegDecoder()
{
    avcodec_free_context(&m_AudioCodecContext);
    avcodec_free_context(&m_VideoCodecContext);
    avformat_free_context(m_FormatContext);
}

void FfmpegDecoder::OpenFile(hstring const& filepath)
{
    if (m_FileOpened && m_FormatContext)
    {
        avformat_close_input(&m_FormatContext);
    }

    m_WavBuffer.clear();
    m_WavBuffer.resize(0);

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

    AVChannelLayout outChLayout;
    av_channel_layout_default(&outChLayout, 2);

    AVSampleFormat outFormat = AV_SAMPLE_FMT_S16;

    error = swr_alloc_set_opts2(
        &m_SwrContext,
        // Output
        &outChLayout,
        outFormat,
        44100,
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

    const AVCodec* subtitlesCodec = nullptr;
    m_SubtitlesStreamIndex = av_find_best_stream(m_FormatContext, AVMEDIA_TYPE_SUBTITLE, -1, -1, &subtitlesCodec, 0);

    if (m_SubtitlesStreamIndex >= 0)
    {
        m_SubtitlesCodecContext = avcodec_alloc_context3(subtitlesCodec);
        if (!m_SubtitlesCodecContext)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Subtitles avcodec_alloc_context3");
            return;
        }

        error = avcodec_parameters_to_context(m_SubtitlesCodecContext, m_FormatContext->streams[m_SubtitlesStreamIndex]->codecpar);
        if (error != 0)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Subtitles avcodec_parameters_to_context");
            return;
        }

        error = avcodec_open2(m_SubtitlesCodecContext, subtitlesCodec, nullptr);
        if (error != 0)
        {
            OutputDebugString(L"FfmpegDecoder::OpenFile Subtitles avcodec_open2");
            return;
        }
    }

    auto frame = av_frame_alloc();
    defer{ av_frame_free(&frame); };

    auto packet = av_packet_alloc();
    defer{ av_packet_free(&packet); };

    std::vector<uint8_t> pcmData;

    while (av_read_frame(m_FormatContext, packet) == 0)
    {
        defer{ av_packet_unref(packet); };

        if (packet->stream_index != m_AudioStreamIndex && packet->stream_index != m_SubtitlesStreamIndex)
        {
            continue;
        }

        if (packet->stream_index == m_SubtitlesStreamIndex)
        {
            int gotSub = 0;
            AVSubtitle subtitle;
            error = avcodec_decode_subtitle2(m_SubtitlesCodecContext, &subtitle, &gotSub, packet);
            if (error < 0)
            {
                OutputDebugString(L"FfmpegDecoder::OpenFile Subtitles avcodec_decode_subtitle2");
                continue;
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
                        subItem.Text = Utils::StringToWString(rect->text);
                    }
                    else if (rect->ass)
                    {
                        ParseAssDialogue(subItem, rect->ass);
                    }
                }

                if (subItem.StartTime == 0 || subItem.EndTime == 0)
                {
                    double subTimeBase = av_q2d(m_FormatContext->streams[m_SubtitlesStreamIndex]->time_base);
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

                m_SubsQueue.push(subItem);
            }
            continue;
        }

        error = avcodec_send_packet(m_AudioCodecContext, packet);
        if (error != 0)
        {
            continue;
        }

        error = avcodec_receive_frame(m_AudioCodecContext, frame);
        if (error != 0)
        {
            continue;
        }

        
        int outSamples = swr_get_out_samples(m_SwrContext, frame->nb_samples);

        if (av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format)))
        {
            uint8_t** buffer = nullptr;
            av_samples_alloc_array_and_samples(&buffer, nullptr, outChLayout.nb_channels, outSamples, outFormat, 0);
            outSamples = swr_convert(
                m_SwrContext,
                buffer,
                outSamples,
                frame->extended_data,
                frame->nb_samples);
            //int dataSize = av_samples_get_buffer_size(nullptr, outChLayout.nb_channels, outSamples, outFormat, 0);
            int dataSize = outSamples * outChLayout.nb_channels * 2;
            pcmData.insert(pcmData.end(), buffer[0], buffer[0] + dataSize);

            if (buffer)
            {
                av_free(buffer[0]);
                av_free(buffer);
            }
        }
        else
        {
            uint8_t* buffer = nullptr;
            av_samples_alloc(&buffer, nullptr, outChLayout.nb_channels, outSamples, outFormat, 0);
            outSamples = swr_convert(
                m_SwrContext,
                &buffer,
                outSamples,
                frame->data,
                frame->nb_samples);
            //int dataSize = av_samples_get_buffer_size(nullptr, outChLayout.nb_channels, outSamples, outFormat, 0);
            int dataSize = outSamples * outChLayout.nb_channels * 2;
            pcmData.insert(pcmData.end(), buffer, buffer + dataSize);

            if (buffer)
            {
                av_free(buffer);
            }
        }
    }

    WavHeader header;
    header.NumChannels = 2;
    header.SampleRate = 44100;
    header.BitsPerSample = 16;
    header.BlockAlign = header.NumChannels * header.BitsPerSample / 8;
    header.ByteRate = header.SampleRate * header.BlockAlign;
    header.DataSize = static_cast<uint32_t>(pcmData.size());
    header.ChunkSize = sizeof(header) - 8 + header.DataSize;

    m_WavBuffer.resize(sizeof(WavHeader) + header.DataSize);
    memcpy(m_WavBuffer.data(), &header, sizeof(WavHeader));
    memcpy(m_WavBuffer.data() + sizeof(WavHeader), pcmData.data(), pcmData.size());

    error = av_seek_frame(m_FormatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile av_seek_frame");
        return;
    }
}

std::vector<uint8_t>& FfmpegDecoder::GetWavBuffer()
{
    return m_WavBuffer;
}

std::queue<SubtitleItem>& FfmpegDecoder::GetSubsQueue()
{
    return m_SubsQueue;
}

VideoFrame FfmpegDecoder::GetNextFrame()
{
    if (m_VideoStreamIndex < 0)
    {
        return {};
    }
            
    auto frame = av_frame_alloc();
    defer{ av_frame_free(&frame); };

    auto packet = av_packet_alloc();
    defer{ av_packet_free(&packet); };

    while (av_read_frame(m_FormatContext, packet) == 0)
    {
        if (packet->stream_index == m_VideoStreamIndex)
        {
            break;
        }

        av_packet_unref(packet);
    }
    defer{ av_packet_unref(packet); };

    int error = avcodec_send_packet(m_VideoCodecContext, packet);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetNextFrame avcodec_send_packet");
        return {};
    }

    error = avcodec_receive_frame(m_VideoCodecContext, frame);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::GetNextFrame avcodec_receive_frame");
        return {};
    }

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
        return {};
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
    //outFrame.FrameTime = av_rescale_q(frame->best_effort_timestamp, m_FormatContext->streams[m_VideoStreamIndex]->time_base, AV_TIME_BASE_Q);
    outFrame.FrameTime = static_cast<double>(frame->pts) * av_q2d(m_FormatContext->streams[m_VideoStreamIndex]->time_base);
    return outFrame;
}

void FfmpegDecoder::Seek(uint64_t time)
{
    if (m_VideoStreamIndex < 0)
    {
        return;
    }

    const int64_t seekTarget = av_rescale_q(
        time * 1000,
        AV_TIME_BASE_Q,
        m_FormatContext->streams[m_VideoStreamIndex]->time_base
    );

    av_seek_frame(m_FormatContext, m_VideoStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_VideoCodecContext);
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
    subItem.Name = Utils::StringToWString(token);

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
    subItem.Text = Utils::StringToWString(token);
}
