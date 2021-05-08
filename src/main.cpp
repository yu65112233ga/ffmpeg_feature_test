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
 
  FILE *pFile;
  char szFilename[32];
  int  y;
 
  // Open file
  sprintf(szFilename, "frame%d.ppm", index);//文件名
  pFile=fopen(szFilename, "wb");
 
  if(pFile==nullptr)
    return;
 
  // Write header
  fprintf(pFile, "P6 %d %d 255", width, height);
 
  // Write pixel data
  for(y=0; y<height; y++)
  {
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  }
 
  // Close file
  fclose(pFile);
 
}

int finish_audio_encoding(AVCodecContext *aud_codec_context, AVFormatContext *outctx, AVStream *audio_st)
{
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    fflush(stdout);

    int ret = avcodec_send_frame(aud_codec_context, NULL);
    if (ret < 0)
        return -1;

    while (true)
    {
        ret = avcodec_receive_packet(aud_codec_context, &pkt);
        if (!ret)
        {
            if (pkt.pts != AV_NOPTS_VALUE)
                pkt.pts = av_rescale_q(pkt.pts, aud_codec_context->time_base, audio_st->time_base);
            if (pkt.dts != AV_NOPTS_VALUE)
                pkt.dts = av_rescale_q(pkt.dts, aud_codec_context->time_base, audio_st->time_base);

            av_write_frame(outctx, &pkt);
            av_packet_unref(&pkt);
        }
        if (ret == -AVERROR(AVERROR_EOF))
            break;
        else if (ret < 0)
            return -1;
    }

    av_write_trailer(outctx);
}

int main() {

    std::string filepath = "./sample_mv.aac";
    AVFormatContext *formatCtx = avformat_alloc_context();
    avformat_open_input(&formatCtx, filepath.c_str(), nullptr, nullptr);
    avformat_find_stream_info(formatCtx, nullptr);
    AVCodecID videoCodecId = AV_CODEC_ID_NONE;
    AVCodecID audioCodecId = AV_CODEC_ID_NONE;


    av_log_set_level(AV_LOG_TRACE);

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
        printf("media type : %s  decoder   %lld   index  %d  \n", mediaType.c_str(), formatCtx->streams[i]->codecpar->codec_id, i);
    }

//    auto videoCodec = avcodec_find_decoder(formatCtx->streams[videoStreamIndex]->codecpar->codec_id);
    auto audioCodec = avcodec_find_decoder(formatCtx->streams[audioStreamIndex]->codecpar->codec_id);
    printf("%lld  \n", formatCtx->duration);
    auto test = av_rescale_q(1995776, formatCtx->streams[audioStreamIndex]->time_base, AV_TIME_BASE_Q);
    int b;
//    auto videoCodecCtx = avcodec_alloc_context3(videoCodec);
//    videoCodecCtx->skip_frame = AVDISCARD_NONREF;
//
//    videoCodecCtx->thread_count = 16;
//    videoCodecCtx->thread_type = 1;
//    avcodec_parameters_to_context(videoCodecCtx, formatCtx->streams[videoStreamIndex]->codecpar);
//    videoCodecCtx->skip_frame = AVDISCARD_NONREF;
//    videoCodecCtx->thread_count = 16;
//    videoCodecCtx->thread_type = 1;
//
//    if (avcodec_open2(videoCodecCtx, videoCodec, NULL) < 0){
//        printf("Failed to open video decoder");
//        return 0;
//    }
//    videoCodecCtx->skip_frame = AVDISCARD_NONREF;
//    videoCodecCtx->thread_count = 16;
//    videoCodecCtx->thread_type = 1;

    auto audioCodecCtx = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(audioCodecCtx, formatCtx->streams[audioStreamIndex]->codecpar);

    if (avcodec_open2(audioCodecCtx, audioCodec, NULL) < 0){
        printf("Failed to open audio decoder");
        return 0;
    }

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

    AVPacket *pkt = av_packet_alloc();
    AVPacket *writePkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    AVFrame *pFrameRGB = av_frame_alloc();
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
    printf("test     9  \n");

    size_t len = 0;
    // ret = av_seek_frame(formatCtx, videoStreamIndex , 1000, AVSEEK_FLAG_ANY);
    // if (ret < 0) {
    //     printf("seek error %d \n", ret);
    //     return -1;
    // }

//    ret =avformat_seek_file(formatCtx, videoStreamIndex, INT64_MIN, 600, INT64_MAX, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        printf("seek error %d \n", ret);
        return -1;
    }

    int i = 0;
    int nb_frams = 0;
    while(av_read_frame(formatCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex) {
//            if (avcodec_send_packet(videoCodecCtx, pkt) >= 0) {
//                printf("decode video no %d packet \n", pkt->pts);
//
//                ret = avcodec_receive_frame(videoCodecCtx, frame);
//                if (ret >= 0) {
//                    i++;
//                    printf("decode video %d frame \n", frame->pkt_pos);
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
//                } else {
//                    printf("read frame error %d  \n", ret);
//                }
//            }
        }
         if (pkt->stream_index == audioStreamIndex) {
             nb_frams++;

             if (avcodec_send_packet(audioCodecCtx, pkt) >= 0) {

                 if (avcodec_receive_frame(audioCodecCtx, frame) >= 0) {
                      printf("decode audio %d frame \n", frame->pkt_pos);

                     // if (frame->pkt_pos > 3653162) {
                     //     continue;
                     // }
                     // ret = avcodec_send_frame(audioEncoderCtx, frame);
                     // if (ret < 0) {
                     //     printf("encode error %d\n", ret);
                     //     exit(1);
                     // }

                     // gotOutput = avcodec_receive_packet(audioEncoderCtx, writePkt);
                     // if (gotOutput == 0) {
                     //     ret = av_write_frame(output_format_context, writePkt);
                     //     if (ret < 0) {
                     //         printf("write frame error %d \n", ret);
                     //         return -1;
                     //     }
                     //     av_packet_unref(writePkt);
                     // }
                 }
             }
         }

        av_packet_unref(pkt);
    }

    // finish_audio_encoding(videoEncoderCtx, output_format_context, video_st);
    // printf("yulinhao   %d     %d \n", audioEncoderCtx->initial_padding, audio_st->codecpar->seek_preroll);
    printf("ttt  %d  \n", nb_frams);
    av_frame_free(&frame);
    avcodec_free_context(&audioCodecCtx);

    return 0;
}