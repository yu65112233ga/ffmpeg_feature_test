//
// Created by liam on 2021/3/5.
//

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}

#include "VideoProcessor.h"

static void decode(AVCodecContext *cdc_ctx, AVFrame *frame, AVPacket *pkt, FILE *fp_out)
{
    int ret = 0;
    int y;

    if ((ret = avcodec_send_packet(cdc_ctx, pkt)) < 0)
    {
        fprintf(stderr, "avcodec_send_packet failed.\n");
        exit(1);
    }

    while ((ret = avcodec_receive_frame(cdc_ctx, frame)) >= 0)
    {
        printf("Write 1 frame.\n");

        for (y = 0; y < frame->height; y++)
            fwrite(&frame->data[0][y * frame->linesize[0]], 1, frame->width, fp_out);
        for (y = 0; y < frame->height / 2; y++)
            fwrite(&frame->data[1][y * frame->linesize[1]], 1, frame->width / 2, fp_out);
        for (y = 0; y < frame->height / 2; y++)
            fwrite(&frame->data[2][y * frame->linesize[2]], 1, frame->width / 2, fp_out);
    }

    if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF))
    {
        fprintf(stderr, "avcodec_receive_packet failed.\n");
        exit(1);
    }
}

void decode_audio(const char *input_file, const char *output_file)
{
    int ret = 0;
    AVCodec *codec = NULL;
    AVCodecContext *cdc_ctx = NULL;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    FILE *fp_in, *fp_out;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecParserContext *parser = NULL;

    if ((codec = avcodec_find_decoder(AV_CODEC_ID_H264)) == NULL)
    {
        fprintf(stderr, "avcodec_find_decoder failed.\n");
        goto ret1;
    }

    if ((cdc_ctx = avcodec_alloc_context3(codec)) == NULL)
    {
        fprintf(stderr, "avcodec_alloc_context3 failed.\n");
        goto ret1;
    }

    if ((ret = avcodec_open2(cdc_ctx, codec, NULL)) < 0)
    {
        fprintf(stderr, "avcodec_open2 failed.\n");
        goto ret2;
    }

    if ((pkt = av_packet_alloc()) == NULL)
    {
        fprintf(stderr, "av_packet_alloc failed.\n");
        goto ret3;
    }

    if ((frame = av_frame_alloc()) == NULL)
    {
        fprintf(stderr, "av_frame_alloc failed.\n");
        goto ret4;
    }

    if ((fp_out = fopen(output_file, "wb")) == NULL)
    {
        fprintf(stderr, "fopen %s failed.\n", output_file);
        goto ret5;
    }

    if ((fmt_ctx = avformat_alloc_context()) == NULL)
    {
        fprintf(stderr, "avformat_alloc_context failed.\n");
        goto ret7;
    }

    if ((ret = avformat_open_input(&fmt_ctx, input_file, NULL, NULL)) < 0)
    {
        fprintf(stderr, "avformat_open_input failed.\n");
        goto ret8;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0)
    {
        fprintf(stderr, "avformat_find_stream_info failed.\n");
        goto ret9;
    }

    while ((ret = av_read_frame(fmt_ctx, pkt)) == 0)
    {
        if (pkt->size > 0)
            decode(cdc_ctx, frame, pkt, fp_out);
    }

    decode(cdc_ctx, frame, NULL, fp_out);

    avformat_close_input(&fmt_ctx);
    avformat_free_context(fmt_ctx);
    fclose(fp_out);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_close(cdc_ctx);
    avcodec_free_context(&cdc_ctx);
    return;

#if 0
    ret8:
	fclose(fp_in);
#else
    ret9:
    avformat_close_input(&fmt_ctx);
    ret8:
    avformat_free_context(fmt_ctx);
#endif
    ret7:
    fclose(fp_out);
    ret5:
    av_frame_free(&frame);
    ret4:
    av_packet_free(&pkt);
    ret3:
    avcodec_close(cdc_ctx);
    ret2:
    avcodec_free_context(&cdc_ctx);
    ret1:
    exit(1);
}