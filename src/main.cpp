extern "C" {
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
    #include "libswresample/swresample.h"
    #include <libavutil/opt.h>
    #include "libavutil/pixfmt.h"
    #include "libavutil/log.h"
}

#include <string>
#include <iostream>
#include <stdio.h>

#define CODEC_CAP_DELAY 0x0020

struct AVCodecContext *pAVCodecCtx_decoder = NULL;
struct AVCodec *pAVCodec_decoder;
struct AVPacket *mAVPacket_decoder;
struct AVFrame *pAVFrame_decoder = NULL;
struct SwsContext* pImageConvertCtx_decoder = NULL;
struct AVFrame *pFrameYUV_decoder = NULL;

#define        ADTS_HEADER_LEN      7;

void SaveFrame(AVFrame *pFrame, int width, int height,int index)
{
 
//   FILE *pFile;
//   char szFilename[32];
//   int  y;
 
//   // Open file
//   sprintf(szFilename, "frame%d.ppm", index);//文件名
//   pFile=fopen(szFilename, "wb");
 
//   if(pFile==nullptr)
//     return;
 
//   // Write header
//   fprintf(pFile, "P6 %d %d 255", width, height);
 
//   // Write pixel data
//   for(y=0; y<height; y++)
//   {
//     fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
//   }
 
//   // Close file
//   fclose(pFile);
 
}

int finish_audio_encoding(AVCodecContext *aud_codec_context, AVFormatContext *outctx, AVStream *audio_st)
{
    // AVPacket pkt;
    // av_init_packet(&pkt);
    // pkt.data = NULL;
    // pkt.size = 0;

    // fflush(stdout);

    // int ret = avcodec_send_frame(aud_codec_context, NULL);
    // if (ret < 0)
    //     return -1;

    // while (true)
    // {
    //     ret = avcodec_receive_packet(aud_codec_context, &pkt);
    //     if (!ret)
    //     {
    //         if (pkt.pts != AV_NOPTS_VALUE)
    //             pkt.pts = av_rescale_q(pkt.pts, aud_codec_context->time_base, audio_st->time_base);
    //         if (pkt.dts != AV_NOPTS_VALUE)
    //             pkt.dts = av_rescale_q(pkt.dts, aud_codec_context->time_base, audio_st->time_base);

    //         av_write_frame(outctx, &pkt);
    //         av_packet_unref(&pkt);
    //     }
    //     if (ret == -AVERROR(AVERROR_EOF))
    //         break;
    //     else if (ret < 0)
    //         return -1;
    // }

    // av_write_trailer(outctx);
}

void encodeCodes() {
    // // initial encoder
    // const char *out_file = "test.aac";
    // auto output_format_context = avformat_alloc_context();
    // avformat_alloc_output_context2(&output_format_context, NULL, nullptr, out_file);

    // if (avio_open((&(output_format_context->pb)), out_file, AVIO_FLAG_WRITE) < 0){
    // 	printf("Failed to open output file!\n");
    // 	return -1;
    // }

    // output_format_context->oformat = av_guess_format(nullptr, out_file, nullptr);

    // auto audio_st = avformat_new_stream(output_format_context, 0);
    // if (audio_st==NULL){
    //     printf("Failed to avformat_new_stream!\n");
    // 	return -1;
    // }

    // auto audioEncoder = avcodec_find_encoder(audioCodec->id);
    // auto audioEncoderCtx = avcodec_alloc_context3(audioEncoder);
    // audioEncoderCtx->initial_padding = 0;

    // avcodec_parameters_to_context(audioEncoderCtx, formatCtx->streams[audioStreamIndex]->codecpar);
    // // audioEncoderCtx->channels       = 2;
    // // audioEncoderCtx->channel_layout = av_get_default_channel_layout(2);
    // // audioEncoderCtx->sample_rate    = audioCodecCtx->sample_rate;
    // // audioEncoderCtx->sample_fmt     = audioEncoder->sample_fmts[0];
    // // audioEncoderCtx->bit_rate       = audioCodecCtx->bit_rate;

    // audio_st->time_base.den = audioCodecCtx->sample_rate;
    // audio_st->time_base.num = 1;

    // if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
    //     audioEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // if (avcodec_open2(audioEncoderCtx, audioEncoder, NULL) < 0){
    //     printf("Failed to open audio encoder");
    //     return 0;
    // }
    // // audioEncoderCtx->initial_padding = 0;
    // audioEncoderCtx->initial_padding = 0;
    // audioEncoderCtx->seek_preroll = 2048;


    // avcodec_parameters_from_context(audio_st->codecpar, audioEncoderCtx);

//     const char *out_file = "test.mov";
//     auto output_format_context = avformat_alloc_context();
//     avformat_alloc_output_context2(&output_format_context, NULL, nullptr, out_file);

//     if (avio_open((&(output_format_context->pb)), out_file, AVIO_FLAG_WRITE) < 0){
// 		printf("Failed to open output file!\n");
// 		return -1;
// 	}

//     output_format_context->oformat = av_guess_format(nullptr, out_file, nullptr);

//     auto video_st = avformat_new_stream(output_format_context, 0);
//     if (video_st==NULL){
//         printf("Failed to avformat_new_stream!\n");
// 		return -1;
// 	}

//     auto videoEncoder = avcodec_find_encoder(videoCodec->id);
//     auto videoEncoderCtx = avcodec_alloc_context3(videoEncoder);

//     avcodec_parameters_to_context(videoEncoderCtx, formatCtx->streams[videoStreamIndex]->codecpar);
//     videoEncoderCtx->time_base = (AVRational){1, 25};
//     videoEncoderCtx->framerate = (AVRational){25, 1};
//     // videoEncoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;

//     video_st->time_base.den = videoCodecCtx->sample_rate;
//     video_st->time_base.num = 1;

//     if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
//         videoEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//    printf("test     2  \n");

//    av_opt_set(videoEncoderCtx->priv_data, "preset", "slow", 0);
//     if (avcodec_open2(videoEncoderCtx, videoEncoder, NULL) < 0){
//         printf("Failed to open video encoder");
//         return 0;
//     }
//    printf("test     4  \n");

//     avcodec_parameters_from_context(video_st->codecpar, videoEncoderCtx);

    int got_frame, gotOutput, ret = 0;
//    printf("test     000 \n");

//     ret = avformat_write_header(output_format_context,NULL);
//     if (ret != 0 && ret != 1) {
//         printf("fail write header  %d \n", ret);
//         return -1;
//     }
//    printf("test     3  \n");

    // resampler
    // SwrContext *resample_context = swr_alloc_set_opts(NULL,
    //                                           av_get_default_channel_layout(audioEncoderCtx->channels),
    //                                           audioEncoderCtx->sample_fmt,
    //                                           audioEncoderCtx->sample_rate,
    //                                           av_get_default_channel_layout(audioCodecCtx->channels),
    //                                           audioCodecCtx->sample_fmt,
    //                                           audioCodecCtx->sample_rate,
    //                                           0, NULL);;

    // if (!resample_context) {
    //     printf("init resample failed \n");
    //     return -1;
    // }

    // if (swr_init(*resample_context) < 0) {
    //     printf("Could not open resample context\n");
    //     swr_free(resample_context);
    //     return -1;
    // }
    //end



    // AVFrame *writeFrame = av_frame_alloc();
    // writeFrame->nb_samples = audioEncoderCtx->frame_size;
    // writeFrame->format = audioEncoderCtx->sample_fmt;
    // writeFrame->channel_layout = audioEncoderCtx->channel_layout;


//    auto img_convert_ctx = sws_getContext(videoCodecCtx->width, videoCodecCtx->height,
//            videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height,
//            AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
//
//    auto numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, videoCodecCtx->width,videoCodecCtx->height);
//    auto out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
//    avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB24,
//            videoCodecCtx->width, videoCodecCtx->height);
}

using namespace std;
int main(int argc, char **argv) {
    if (argc <= 2) {
        printf("enter decode filepath & output filepath\n");
        return -1;
    }
    std::string filepath(argv[1]);
    std::string ofilepath(argv[2]);
    bool isHW = false;
    if (argc >= 4) {
        std::string tmp(argv[3]);
        if (tmp == "-hw") {
            isHW = true;
        }
    }

    AVFormatContext *formatCtx = avformat_alloc_context();
    avformat_open_input(&formatCtx, filepath.c_str(), nullptr, nullptr);
    AVCodecID videoCodecId = AV_CODEC_ID_NONE;
    AVCodecID audioCodecId = AV_CODEC_ID_NONE;

    // av_log_set_level(AV_LOG_TRACE);

    int videoStreamIndex = -1;
    int audioStreamIndex = -1; 

    for (int i = 0; i < formatCtx->nb_streams; i++) {
        std::string mediaType = "unkown";
        switch (formatCtx->streams[i]->codecpar->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
            mediaType = "video";
            videoStreamIndex = i;
            break;
        case AVMEDIA_TYPE_AUDIO:
            mediaType = "audio";
            audioStreamIndex = i;
            break;
        default:
            break;
        }
        printf("media type : %s  decoder   %d   index  %d  \n", mediaType.c_str(), formatCtx->streams[i]->codecpar->codec_id, i);
    }

    std::string codecName = isHW ? "h264_nvenc" : "libx264";
    std::string decodeName = isHW ? "h264_cuvid" : "libx264";

    auto videoCodec = avcodec_find_decoder_by_name(decodeName.c_str());
    if (!videoCodec) {
        printf("Codec '%s' not found\n", decodeName.c_str());
        return -1;
    }
    // auto audioCodec = avcodec_find_decoder(formatCtx->streams[audioStreamIndex]->codecpar->codec_id);

    // video codec context
    auto videoCodecCtx = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(videoCodecCtx, formatCtx->streams[videoStreamIndex]->codecpar);
    if (isHW) {
        videoCodecCtx->time_base = formatCtx->streams[videoStreamIndex]->time_base;
        videoCodecCtx->framerate = formatCtx->streams[videoStreamIndex]->avg_frame_rate;
        videoCodecCtx->pix_fmt = AV_PIX_FMT_CUDA;
    }

    // audio codec context
    // auto audioCodecCtx = avcodec_alloc_context3(audioCodec);
    // avcodec_parameters_to_context(audioCodecCtx, formatCtx->streams[audioStreamIndex]->codecpar);

    // if (avcodec_open2(audioCodecCtx, audioCodec, NULL) < 0){
    //     printf("Failed to open audio decoder");
    //     return 0;
    // }

    // initial encoder
    auto encoderCodec = avcodec_find_encoder_by_name(codecName.c_str());
    if (!encoderCodec) {
        printf("Codec '%s' not found\n", codecName.c_str());
        return -1;
    }

    auto encoderCtx = avcodec_alloc_context3(encoderCodec);
    if (!encoderCtx) {
        printf("Could not allocate video codec context\n");
        return -1;
    }

    avcodec_parameters_to_context(encoderCtx, formatCtx->streams[videoStreamIndex]->codecpar);

    encoderCtx->width = videoCodecCtx->width;
    encoderCtx->height = videoCodecCtx->height;
    encoderCtx->time_base = formatCtx->streams[videoStreamIndex]->time_base;
    encoderCtx->framerate  = formatCtx->streams[videoStreamIndex]->avg_frame_rate;

    encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (isHW) {
        encoderCtx->pix_fmt = AV_PIX_FMT_CUDA;
    }
    AVBufferRef *hw_device_ctx = NULL;
    if (isHW) {
        AVHWDeviceType dev_type = AV_HWDEVICE_TYPE_CUDA;

        AVDictionary *opt = nullptr;
        std::string device = "0";
        int err = 0;
        if (!hw_device_ctx) {
            hw_device_ctx = av_hwdevice_ctx_alloc(dev_type);
            err = av_hwdevice_ctx_create(&hw_device_ctx, dev_type, device.data(), opt, 0);

            if (err < 0) {
                err = av_hwdevice_ctx_create(&hw_device_ctx, dev_type, NULL, opt, 0);
                if (err < 0) {
                    av_dict_free(&opt);
                    char err_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                    printf("Failed to create a CUDA device. Error code: %s", av_make_error_string(err_buffer, sizeof(err_buffer), err));
                    return err;
                }
            }
        }

        av_dict_free(&opt);
        auto set_hwframe_ctx = [=](AVCodecContext *ctx, AVBufferRef *in_hw_device_ctx) -> int {
            AVBufferRef *hw_frames_ref;
            AVHWFramesContext *frames_ctx = NULL;
            int err = 0;

            if (!(hw_frames_ref = av_hwframe_ctx_alloc(in_hw_device_ctx))) {
                printf("Failed to create CUDA frame context.");
                return -1;
            }

            frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
            frames_ctx->format = AV_PIX_FMT_CUDA;
            frames_ctx->sw_format = AV_PIX_FMT_NV12;
            frames_ctx->width = ctx->width;
            frames_ctx->height = ctx->height;

            if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
                char err_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                printf("Failed to initialize CUDA frame context. Error code: %s\n", av_make_error_string(err_buffer, sizeof(err_buffer), err));
                av_buffer_unref(&hw_frames_ref);
                return err;
            }

            ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
            if (!ctx->hw_frames_ctx) {
                err = AVERROR(ENOMEM);
            }

            av_buffer_unref(&hw_frames_ref);
            return err;
        };

        if ((err = set_hwframe_ctx(videoCodecCtx, hw_device_ctx)) < 0) {
            printf("Failed to init CUDA frame context!");
            return err;
        }
        encoderCtx->hw_frames_ctx = av_buffer_ref(videoCodecCtx->hw_frames_ctx);
    }

    if (avcodec_open2(videoCodecCtx, videoCodec, NULL) < 0){
        printf("Failed to open video decoder");
        return 0;
    }

    if (avcodec_open2(encoderCtx, encoderCodec, NULL)) {
        printf("Could not open codec\n");
        return -1;
    }

    AVFormatContext *oformatCtx = avformat_alloc_context();

    auto oformat = av_guess_format(nullptr, ofilepath.c_str(), nullptr);
    if (!oformat) {
        oformat = av_guess_format("mp4", ofilepath.c_str(), nullptr);
    }
    if (!oformat) {
        printf("[%s:%d]can't deduce oformat, filename: %s", __FILE__, __LINE__, ofilepath.c_str());
        return -1;
    }

    if (avformat_alloc_output_context2(&oformatCtx, oformat, nullptr, ofilepath.c_str()) < 0) {
        oformatCtx = nullptr;
        return -1;
    }

    oformatCtx->oformat->video_codec = encoderCtx->codec->id;

    auto ostream = avformat_new_stream(oformatCtx, encoderCodec);
    if (!ostream) {
        printf("Failed to add video stream!");
        return AVERROR(ENOMEM);
    }

    ostream->codecpar->codec_id = encoderCtx->codec->id;

    // Stream id
    ostream->id = oformatCtx->nb_streams - 1;
    ostream->avg_frame_rate = formatCtx->streams[videoStreamIndex]->avg_frame_rate;
    ostream->time_base = formatCtx->streams[videoStreamIndex]->time_base;
    if (avcodec_parameters_from_context(ostream->codecpar, encoderCtx)) {
        return -1;
    }

    if (avio_open(&oformatCtx->pb, ofilepath.c_str(), AVIO_FLAG_WRITE) <0) {
        printf("open output file fail\n");
        return -1;
    }

    AVDictionary *opt = nullptr;
    if (avformat_write_header(oformatCtx, &opt) < 0) {
        printf("write header fail \n");
        return -1;
    }

    AVPacket *outPkt = av_packet_alloc();
    if (!outPkt) {
        return -1;
    }

    AVPacket *pkt = av_packet_alloc();
    AVPacket *writePkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    if (isHW) {
        frame->hw_frames_ctx = av_buffer_ref(encoderCtx->hw_frames_ctx);
    }

    AVFrame *pFrameRGB = av_frame_alloc();
    int ret = 0;

    int nb_frams = 0;
    while(av_read_frame(formatCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(videoCodecCtx, pkt) >= 0) {
                // printf("decode video no. %ld packet \n", pkt->pos);

                ret = avcodec_receive_frame(videoCodecCtx, frame);
                if (ret >= 0) {
                    nb_frams++;
                    printf("decode video %d frame \n", nb_frams);


                    // encode frame
                    auto ret = avcodec_send_frame(encoderCtx, frame);
                    if (ret < 0) {
                        printf("encode failed\n");
                        continue;
                    }

                    while(ret >= 0) {
                        ret = avcodec_receive_packet(encoderCtx, outPkt);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break;
                        } else if (ret < 0) {
                            printf("encode outPkt fail\n");
                            return -1;
                        }

                        outPkt->stream_index = ostream->index;
                        outPkt->pts = av_rescale_q(outPkt->pts - 0, encoderCtx->time_base, ostream->time_base);
                        outPkt->dts = av_rescale_q(outPkt->dts - 0, encoderCtx->time_base, ostream->time_base);

                        outPkt->duration = 0;   // av_rescale_q(1, videoEncoderCtx->time_base, ostream->time_base);

                        outPkt->duration = encoderCtx->time_base.den * encoderCtx->framerate.den / encoderCtx->framerate.num;

                        ret = av_interleaved_write_frame(oformatCtx, outPkt);
                        if (ret < 0) {
                            printf("write frame fail\n");
                            break;
                        }
                    }


                
//                    sws_scale(img_convert_ctx,
//                        (uint8_t const * const *) frame->data,
//                        frame->linesize, 0, videoCodecCtx->height, pFrameRGB->data,
//                        pFrameRGB->linesize);
//                    SaveFrame(pFrameRGB, videoCodecCtx->width,videoCodecCtx->height,i);
//
//
//                    videoCodecCtx->skip_frame = AVDISCARD_DEFAULT;
//                     if (i <= 10) {
//                         SaveAsBMP(frame, videoCodecCtx->width, videoCodecCtx->height, i, 24);
//                         i++;
//                     }
//                     ret = avcodec_send_frame(videoEncoderCtx, frame);
//                     if (ret < 0) {
//                         printf("encode error %d\n", ret);
//                         exit(1);
//                     }
//
//                     gotOutput = avcodec_receive_packet(videoEncoderCtx, writePkt);
//                     if (gotOutput == 0) {
//                         ret = av_write_frame(output_format_context, writePkt);
//                         if (ret < 0) {
//                             printf("write frame error %d \n", ret);
//                             return -1;
//                         }
//                         av_packet_unref(writePkt);
//                     }
                } else {
                    printf("read frame error %d  \n", ret);
                }
            }
        }

        if (pkt->stream_index == audioStreamIndex) {
        //  nb_frams++;

        //  if (avcodec_send_packet(audioCodecCtx, pkt) >= 0) {

        //      if (avcodec_receive_frame(audioCodecCtx, frame) >= 0) {
        //           printf("decode audio %d frame \n", frame->pkt_pos);

        //          // if (frame->pkt_pos > 3653162) {
        //          //     continue;
        //          // }
        //          // ret = avcodec_send_frame(audioEncoderCtx, frame);
        //          // if (ret < 0) {
        //          //     printf("encode error %d\n", ret);
        //          //     exit(1);
        //          // }

        //          // gotOutput = avcodec_receive_packet(audioEncoderCtx, writePkt);
        //          // if (gotOutput == 0) {
        //          //     ret = av_write_frame(output_format_context, writePkt);
        //          //     if (ret < 0) {
        //          //         printf("write frame error %d \n", ret);
        //          //         return -1;
        //          //     }
        //          //     av_packet_unref(writePkt);
        //          // }
        //      }
        //  }
        }

        av_packet_unref(pkt);
    }


    if (av_write_trailer(oformatCtx) < 0) {
        printf("Could not av_write_trailer of stream\n");
    }

    avio_flush(oformatCtx->pb);

    avio_close(oformatCtx->pb);

    printf("number of frames:  %d  \n", nb_frams);
    av_frame_free(&frame);
//    avcodec_free_context(&audioCodecCtx);

    return 0;
}