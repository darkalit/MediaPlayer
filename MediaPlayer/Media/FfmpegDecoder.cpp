#include "pch.h"
#include "FfmpegDecoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
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
}

void FfmpegDecoder::OpenFile(hstring const& filepath)
{
    int error = avformat_open_input(&m_FormatContext, to_string(filepath).c_str(), nullptr, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avformat_open_input");
        return;
    }
    defer{ avformat_close_input(&m_FormatContext); };

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
        OutputDebugString(L"FfmpegDecoder::OpenFile av_find_best_stream");
        return;
    }

    m_AudioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!m_AudioCodecContext)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avcodec_alloc_context3");
        return;
    }

    error = avcodec_parameters_to_context(m_AudioCodecContext, m_FormatContext->streams[m_AudioStreamIndex]->codecpar);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avcodec_parameters_to_context");
        return;
    }

    error = avcodec_open2(m_AudioCodecContext, audioCodec, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile avcodec_open2");
        return;
    }

    error = swr_alloc_set_opts2(
        &m_SwrContext,
        // Output
        &m_AudioCodecContext->ch_layout,
        AV_SAMPLE_FMT_S16,
        44100,
        // Input
        &m_AudioCodecContext->ch_layout,
        m_AudioCodecContext->sample_fmt,
        m_AudioCodecContext->sample_rate,
        0, nullptr);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile swr_alloc_set_opts2");
        return;
    }

    error = swr_init(m_SwrContext);
    if (error != 0)
    {
        OutputDebugString(L"FfmpegDecoder::OpenFile swr_init");
        return;
    }

    auto frame = av_frame_alloc();
    defer{ av_frame_free(&frame); };

    auto packet = av_packet_alloc();
    defer{ av_packet_free(&packet); };

    std::vector<uint8_t> pcmData;

    while (av_read_frame(m_FormatContext, packet) == 0)
    {
        defer{ av_packet_unref(packet); };

        if (packet->stream_index != m_AudioStreamIndex)
        {
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

        uint8_t* buffer = nullptr;
        int outSamples = swr_get_out_samples(m_SwrContext, frame->nb_samples);
        av_samples_alloc(&buffer, nullptr, 2, outSamples, AV_SAMPLE_FMT_S16, 0);

        outSamples = swr_convert(
            m_SwrContext,
            &buffer,
            outSamples,
            frame->data,
            frame->nb_samples);

        int dataSize = outSamples * 2 * 2;
        pcmData.insert(pcmData.end(), buffer, buffer + dataSize);
        av_free(buffer);
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
}

std::vector<uint8_t>& FfmpegDecoder::GetWavBuffer()
{
    return m_WavBuffer;
}
