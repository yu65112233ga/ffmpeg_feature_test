//
// Created by liam on 23-3-23.
//
#include "flashSDK.h"

static int nb_frams = 0;
static AVBufferRef *hw_device_ctx = NULL;
static FILE *output_file = NULL;

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_CUDA)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}
flashResult flashSDK::init() {
    return FLASH_RESULT_OK;
}

static int decode_write(AVCodecContext *avctx, AVPacket *packet)
{
    AVFrame *frame = NULL, *sw_frame = NULL;
    AVFrame *tmp_frame = NULL;
    uint8_t *buffer = NULL;
    int size;
    int ret = 0;

    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    while (1) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
        nb_frams++;
        printf("decode video %d frame \n", nb_frams);
        fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_freep(&buffer);
        if (ret < 0)
            return ret;
    }
}
flashResult flashSDK::start() {
    int ret = 0;
    std::string codecName = "h264_nvenc";
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
    DecoderContext curCtx;
    {
        std::lock_guard<std::mutex> lk(decoderCtxesMtx);
        curCtx = decoderCtxes.begin()->second;
    }
    auto videoCodecCtx = curCtx.videocodecCtx;
    auto videoStreamIndex = commonDecoderCtx.videoStreamIndex;
    bool isHW = true;

    avcodec_parameters_to_context(encoderCtx, curCtx.formatCtx->streams[videoStreamIndex]->codecpar);

    encoderCtx->width = videoCodecCtx->width;
    encoderCtx->height = videoCodecCtx->height;
    encoderCtx->time_base = curCtx.formatCtx->streams[videoStreamIndex]->time_base;
    encoderCtx->framerate  = curCtx.formatCtx->streams[videoStreamIndex]->avg_frame_rate;

//    encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
//    if (isHW) {
        encoderCtx->pix_fmt = AV_PIX_FMT_CUDA;
//    }
//    encoderCtx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    AVBufferRef *hw_device_ctx = nullptr;
    if (isHW) {
        AVHWDeviceType dev_type = AV_HWDEVICE_TYPE_CUDA;

        AVDictionary *opt = nullptr;
        std::string device = "0";
        int err = 0;
        if (!hw_device_ctx) {
            err = av_hwdevice_ctx_create(&hw_device_ctx, dev_type, device.data(), opt, 0);

            if (err < 0) {
                err = av_hwdevice_ctx_create(&hw_device_ctx, dev_type, NULL, opt, 0);
                if (err < 0) {
                    av_dict_free(&opt);
                    char err_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
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

        if ((err = set_hwframe_ctx(encoderCtx, hw_device_ctx)) < 0) {
            printf("Failed to init CUDA frame context!");
            return err;
        }

//        encoderCtx->hw_frames_ctx = av_buffer_ref(videoCodecCtx->hw_frames_ctx);
    }
//    encoderCtx->get_format = get_hw_format;
    if (avcodec_open2(encoderCtx, encoderCodec, NULL)) {
        printf("Could not open codec\n");
        return -1;
    }

    AVFormatContext *oformatCtx = avformat_alloc_context();

    auto oformat = av_guess_format(nullptr, outputPath.c_str(), nullptr);
    if (!oformat) {
        oformat = av_guess_format("mp4", outputPath.c_str(), nullptr);
    }
    if (!oformat) {
        printf("[%s:%d]can't deduce oformat, filename: %s", __FILE__, __LINE__, outputPath.c_str());
        return -1;
    }

    if (avformat_alloc_output_context2(&oformatCtx, oformat, nullptr, outputPath.c_str()) < 0) {
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
    ostream->avg_frame_rate = curCtx.formatCtx->streams[videoStreamIndex]->avg_frame_rate;
    ostream->time_base = curCtx.formatCtx->streams[videoStreamIndex]->time_base;
    if (avcodec_parameters_from_context(ostream->codecpar, encoderCtx)) {
        return -1;
    }

    if (avio_open(&oformatCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) <0) {
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

//    if (isHW) {
//        frame->hw_frames_ctx = av_buffer_ref(videoCodecCtx->hw_frames_ctx);
//    }

//    AVFrame *pFrameRGB = av_frame_alloc();

//    int nb_frams = 0;
//    auto packet = av_packet_alloc();
//    int ret =0;
//    /* actual decoding and dump the raw data */
//    while (ret >= 0) {
//        if ((ret = av_read_frame(curCtx.formatCtx, packet)) < 0)
//            break;
//
//        if (packet->stream_index == videoStreamIndex)
//            ret = decode_write(videoCodecCtx, packet);
//
//        av_packet_unref(packet);
//    }
//    return FLASH_RESULT_OK;
    while(av_read_frame(curCtx.formatCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(videoCodecCtx, pkt) >= 0) {
                // printf("decode video no. %ld packet \n", pkt->pos);

                ret = avcodec_receive_frame(videoCodecCtx, frame);
                if (ret >= 0) {
                    nb_frams++;
                    printf("decode video %d frame \n", nb_frams);


//                    // encode frame
//                    auto ret = avcodec_send_frame(encoderCtx, frame);
//                    if (ret < 0) {
//                        printf("encode failed\n");
//                        continue;
//                    }
//
//                    while(ret >= 0) {
//                        ret = avcodec_receive_packet(encoderCtx, outPkt);
//                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//                            break;
//                        } else if (ret < 0) {
//                            printf("encode outPkt fail\n");
//                            return -1;
//                        }
//
//                        outPkt->stream_index = ostream->index;
//                        outPkt->pts = av_rescale_q(outPkt->pts - 0, encoderCtx->time_base, ostream->time_base);
//                        outPkt->dts = av_rescale_q(outPkt->dts - 0, encoderCtx->time_base, ostream->time_base);
//
//                        outPkt->duration = 0;   // av_rescale_q(1, videoEncoderCtx->time_base, ostream->time_base);
//
//                        outPkt->duration = encoderCtx->time_base.den * encoderCtx->framerate.den / encoderCtx->framerate.num;
//
//                        ret = av_interleaved_write_frame(oformatCtx, outPkt);
//                        if (ret < 0) {
//                            printf("write frame fail\n");
//                            break;
//                        }
//                    }
//


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

//        if (pkt->stream_index == audioStreamIndex) {
//            //  nb_frams++;
//
//            //  if (avcodec_send_packet(audioCodecCtx, pkt) >= 0) {
//
//            //      if (avcodec_receive_frame(audioCodecCtx, frame) >= 0) {
//            //           printf("decode audio %d frame \n", frame->pkt_pos);
//
//            //          // if (frame->pkt_pos > 3653162) {
//            //          //     continue;
//            //          // }
//            //          // ret = avcodec_send_frame(audioEncoderCtx, frame);
//            //          // if (ret < 0) {
//            //          //     printf("encode error %d\n", ret);
//            //          //     exit(1);
//            //          // }
//
//            //          // gotOutput = avcodec_receive_packet(audioEncoderCtx, writePkt);
//            //          // if (gotOutput == 0) {
//            //          //     ret = av_write_frame(output_format_context, writePkt);
//            //          //     if (ret < 0) {
//            //          //         printf("write frame error %d \n", ret);
//            //          //         return -1;
//            //          //     }
//            //          //     av_packet_unref(writePkt);
//            //          // }
//            //      }
//            //  }
//        }

        av_packet_unref(pkt);
    }


//    if (av_write_trailer(oformatCtx) < 0) {
//        printf("Could not av_write_trailer of stream\n");
//    }
//
//    avio_flush(oformatCtx->pb);
//
//    avio_close(oformatCtx->pb);

    printf("number of frames:  %d  \n", nb_frams);
    av_frame_free(&frame);

    return FLASH_RESULT_OK;
}

flashResult flashSDK::stop() {
    return FLASH_RESULT_OK;
}

flashResult flashSDK::addInput(std::string inputPath, int index) {
    DecoderContext curCtx;
    int ret;
    AVStream *video = NULL;
    const AVCodec *decoder = NULL;
    int i;
    av_log_set_level(AV_LOG_DEBUG);

    /* open the input file */
    if (avformat_open_input(&curCtx.formatCtx, "/home/liam/code/ffmpegTool/flashSDK/proj/resource/video92_1080p.mp4", NULL, NULL) != 0) {
        fprintf(stderr, "Cannot open input file '%s'\n", inputPath.c_str());
        return -1;
    }

    if (avformat_find_stream_info(curCtx.formatCtx, NULL) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }

    /* find the video stream information */
    ret = av_find_best_stream(curCtx.formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, const_cast<AVCodec **>(&decoder), 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    commonDecoderCtx.videoStreamIndex = ret;

    decoder = avcodec_find_decoder_by_name("h264_cuvid");
    if(!decoder) {
        printf("avcodec_find_decoder_by_name() failed");
        return -1;
    }

    if (!(curCtx.videocodecCtx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);

    video = curCtx.formatCtx->streams[commonDecoderCtx.videoStreamIndex];
    if (avcodec_parameters_to_context(curCtx.videocodecCtx, video->codecpar) < 0)
        return -1;

    if (hw_decoder_init(curCtx.videocodecCtx, AV_HWDEVICE_TYPE_CUDA) < 0)
        return -1;

    if ((ret = avcodec_open2(curCtx.videocodecCtx, decoder, NULL)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", commonDecoderCtx.videoStreamIndex);
        return -1;
    }

//    commonDecoderCtx.hwDeviceCtx = hwDeviceCtx;
//    curCtx.videocodecCtx = pVideoDecCtx;
    std::lock_guard<std::mutex> lk(decoderCtxesMtx);
    decoderCtxes.emplace(index, curCtx);
    return FLASH_RESULT_OK;
    av_log_set_level(AV_LOG_DEBUG);
    curCtx.inputPath = inputPath;
    AVCodec *videoCodec = nullptr;
    if (avformat_open_input(&curCtx.formatCtx, curCtx.inputPath.c_str(), nullptr, nullptr) < 0){
        printf("Failed to open video %s\n", curCtx.inputPath.c_str());
        return FLASH_RESULT_ERROR;
    }
    if (avformat_find_stream_info(curCtx.formatCtx, NULL) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }
    ret = av_find_best_stream(curCtx.formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, const_cast<AVCodec **>(&videoCodec), 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    commonDecoderCtx.videoStreamIndex = ret;
//    if (!commonDecoderCtx.isActive) {
//        for (int i = 0; i < curCtx.formatCtx->nb_streams; i++) {
//            std::string mediaType = "unkown";
//            switch (curCtx.formatCtx->streams[i]->codecpar->codec_type) {
//                case AVMEDIA_TYPE_VIDEO:
//                    mediaType = "video";
//                    commonDecoderCtx.videoStreamIndex = i;
//                    break;
//                case AVMEDIA_TYPE_AUDIO:
//                    mediaType = "audio";
//                    commonDecoderCtx.audioStreamIndex = i;
//                    break;
//                default:
//                    break;
//            }
//            printf("media type : %s  decoder   %lld   index  %d  \n", mediaType.c_str(), curCtx.formatCtx->streams[i]->codecpar->codec_id, i);
//        }
//        commonDecoderCtx.isActive = true;
//    }

    auto codecpar = curCtx.formatCtx->streams[commonDecoderCtx.videoStreamIndex]->codecpar;
//    videoCodec = avcodec_find_decoder(codecpar->codec_id);
//    curCtx.videocodecCtx = avcodec_alloc_context3(videoCodec);
//    avcodec_parameters_to_context(curCtx.videocodecCtx, codecpar);
//
//    if (avcodec_open2(curCtx.videocodecCtx, videoCodec, NULL) < 0){
//        printf("Failed to open video decoder");
//        return FLASH_RESULT_ERROR;
//    }

    AVBufferRef *hwDeviceCtx = commonDecoderCtx.hwDeviceCtx;

    auto pVideoDecCtx = curCtx.videocodecCtx;
    auto videoStream = curCtx.formatCtx->streams[commonDecoderCtx.videoStreamIndex];
    
    auto try_open_hw_decoder = [&videoStream, &pVideoDecCtx, &hwDeviceCtx]() {
        AVCodec *pVideoAVCodec = nullptr;
        auto set_hwframe_ctx = [=](AVCodecContext *videoDecodeCtx, AVBufferRef *hwDevCtx) -> int {
            AVBufferRef *hw_frames_ref;
            AVHWFramesContext *frames_ctx = nullptr;
            int err = 0;

            if (!(hw_frames_ref = av_hwframe_ctx_alloc(hwDevCtx))) {
                printf("Failed to create CUDA frame context.");
                return -1;
            }

            frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
            frames_ctx->format = AV_PIX_FMT_CUDA;
//            frames_ctx->sw_format = AV_PIX_FMT_NV12;
            frames_ctx->width = videoDecodeCtx->width;
            frames_ctx->height = videoDecodeCtx->height;

            if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
                char err_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                printf("Failed to initialize CUDA frame context. Error code: %s\n", av_make_error_string(err_buffer, sizeof(err_buffer), err));
                av_buffer_unref(&hw_frames_ref);
                return err;
            }
            videoDecodeCtx->pix_fmt = AV_PIX_FMT_CUDA;
            videoDecodeCtx->hw_device_ctx = av_buffer_ref(hwDevCtx);
            if(!videoDecodeCtx->hw_device_ctx) {
                printf("unable to refer hw_device_ctx");
                err = AVERROR(EINVAL);
            }

            videoDecodeCtx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
            if (!videoDecodeCtx->hw_frames_ctx) {
                err = AVERROR(ENOMEM);
            }

            av_buffer_unref(&hw_frames_ref);
            return err;
        };

        if(videoStream->codecpar->codec_id != AV_CODEC_ID_H264) {
            return -1;
        }
        pVideoAVCodec = avcodec_find_decoder_by_name("h264_cuvid");
        if(!pVideoAVCodec) {
            printf("avcodec_find_decoder_by_name() failed");
            return -1;
        }
        pVideoDecCtx = avcodec_alloc_context3(pVideoAVCodec);
        if(!pVideoDecCtx) {
            printf("avcodec_alloc_context3() failed");
            return -1;
        }
        // copy codec parameters from input stream to output codec context
        avcodec_parameters_to_context(pVideoDecCtx, videoStream->codecpar);
        int ret = 0;
        if((ret = av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0)) < 0) {
            printf("av_hwdevice_ctx_create() failed, ret = %d", ret);
            if((ret = av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0)) < 0) {
                printf("av_hwdevice_ctx_create() retry failed, ret = %d", ret);
                return -1;
            }
        }
//        pVideoDecCtx->time_base = videoStream->time_base;
//        pVideoDecCtx->framerate = videoStream->avg_frame_rate;
//        pVideoDecCtx->pkt_timebase = videoStream->time_base;
        pVideoDecCtx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
//        if (set_hwframe_ctx(pVideoDecCtx, hwDeviceCtx) < 0) {
//            printf("Failed to init CUDA frame context!");
//            return -1;
//        }
        if((ret = avcodec_open2(pVideoDecCtx, pVideoAVCodec, nullptr)) < 0) {
            printf("avcodec_open2() failed, ret = %d", ret);
            return -1;
        }
//        if(hwDeviceCtx) av_buffer_unref(&hwDeviceCtx);
        return 0;
    };

    if (false) {
        auto pVideoAVCodec = avcodec_find_decoder_by_name("h264");
        if(!pVideoAVCodec) {
            printf("avcodec_find_decoder_by_name() failed");
            return -1;
        }
        pVideoDecCtx = avcodec_alloc_context3(pVideoAVCodec);
        if(!pVideoDecCtx) {
            printf("avcodec_alloc_context3() failed");
            return -1;
        }
        // copy codec parameters from input stream to output codec context
        avcodec_parameters_to_context(pVideoDecCtx, videoStream->codecpar);

        pVideoDecCtx->time_base = videoStream->time_base;
        pVideoDecCtx->framerate = videoStream->avg_frame_rate;
        pVideoDecCtx->pkt_timebase = videoStream->time_base;

        if(avcodec_open2(pVideoDecCtx, pVideoAVCodec, nullptr) < 0) {
            printf("avcodec_open2() failed, ret");
            return -1;
        }
    }


    auto pVideoAVCodec = avcodec_find_decoder_by_name("h264_cuvid");
    if(!pVideoAVCodec) {
        printf("avcodec_find_decoder_by_name() failed");
        return -1;
    }
    pVideoDecCtx = avcodec_alloc_context3(pVideoAVCodec);
    if(!pVideoDecCtx) {
        printf("avcodec_alloc_context3() failed");
        return -1;
    }
    // copy codec parameters from input stream to output codec context
    avcodec_parameters_to_context(pVideoDecCtx, videoStream->codecpar);
    pVideoDecCtx->get_format  = get_hw_format;
    if((ret = av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0)) < 0) {
        printf("av_hwdevice_ctx_create() failed, ret = %d", ret);
        if((ret = av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0)) < 0) {
            printf("av_hwdevice_ctx_create() retry failed, ret = %d", ret);
            return -1;
        }
    }
//        pVideoDecCtx->time_base = videoStream->time_base;
//        pVideoDecCtx->framerate = videoStream->avg_frame_rate;
//        pVideoDecCtx->pkt_timebase = videoStream->time_base;
    pVideoDecCtx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
//        if (set_hwframe_ctx(pVideoDecCtx, hwDeviceCtx) < 0) {
//            printf("Failed to init CUDA frame context!");
//            return -1;
//        }
    if((ret = avcodec_open2(pVideoDecCtx, pVideoAVCodec, nullptr)) < 0) {
        printf("avcodec_open2() failed, ret = %d", ret);
        return -1;
    }

    auto videoStreamIndex = commonDecoderCtx.videoStreamIndex;
    auto videoCodecCtx = curCtx.videocodecCtx;
    bool isHW = true;

    return FLASH_RESULT_OK;
}
flashResult flashSDK::setOutput(int inputNum, long duration, std::string outputPath) {
    this->inputNum = inputNum;
    this->outputPath = outputPath;
    return FLASH_RESULT_OK;
};