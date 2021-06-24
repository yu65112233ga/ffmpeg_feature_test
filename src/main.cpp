extern "C" {
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
    #include "libswresample/swresample.h"
    #include <libavutil/opt.h>
    #include "libavutil/pixfmt.h"
    #include "libavutil/log.h"
    #include <libavutil/imgutils.h>
}

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>
#include <stdio.h>
#include <thread>
#include <deque>
#include <mutex>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 720;
const unsigned int SCR_HEIGHT = 1080;

struct AVCodecContext *pAVCodecCtx_decoder = NULL;
struct AVCodec *pAVCodec_decoder;
struct AVPacket *mAVPacket_decoder;
struct AVFrame *pAVFrame_decoder = NULL;
struct SwsContext* pImageConvertCtx_decoder = NULL;
struct AVFrame *pFrameYUV_decoder = NULL;

std::deque<AVFrame* > framesQueue;
int global_width = 0;
int global_height = 0;
std::mutex mtx;

int renderVideo()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("4.2.texture.vs", "4.2.texture.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
            // positions          // colors           // texture coords
            0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // top right
            0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, // bottom left
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f  // top left
    };
    unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    unsigned int textureid;
    int frameNum = 0;
    bool isNewFrame = false;
    AVFrame *curFrame = nullptr;
    bool isLoop = true;

    ourShader.use(); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    glUniform1i(glGetUniformLocation(ourShader.ID, "texture1"), 0);
    // or set it via the texture class
    ourShader.setInt("texture2", 1);
    // render loop
    // -----------
    int a = 0;
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // load and create a texture
        // -------------------------
        mtx.lock();
        if (framesQueue.size() > frameNum) {
            if (curFrame != nullptr) {
                av_free(curFrame);
                curFrame = nullptr;
            }
            curFrame = framesQueue[frameNum];
            frameNum++;
            isNewFrame = true;
        } else {
            isNewFrame = false;
        }
        mtx.unlock();

        if (isNewFrame) {
            // ---------
            glGenTextures(1, &textureid);
            glBindTexture(GL_TEXTURE_2D, textureid);
            // set the texture wrapping parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // load image, create texture and generate mipmaps

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global_width, global_height, 0, GL_RGB, GL_UNSIGNED_BYTE, curFrame->data[0]);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureid);

        // render container
        ourShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
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

int decodeVideo() {

    std::string filepath = "./sample_mv.mp4";
    AVFormatContext *formatCtx = avformat_alloc_context();
    avformat_open_input(&formatCtx, filepath.c_str(), nullptr, nullptr);
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

    auto videoCodec = avcodec_find_decoder(formatCtx->streams[videoStreamIndex]->codecpar->codec_id);
    auto audioCodec = avcodec_find_decoder(formatCtx->streams[audioStreamIndex]->codecpar->codec_id);

    // video codec context
    auto videoCodecCtx = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(videoCodecCtx, formatCtx->streams[videoStreamIndex]->codecpar);

    if (avcodec_open2(videoCodecCtx, videoCodec, NULL) < 0){
        printf("Failed to open video decoder");
        return 0;
    }

    // audio codec context
    auto audioCodecCtx = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(audioCodecCtx, formatCtx->streams[audioStreamIndex]->codecpar);

    if (avcodec_open2(audioCodecCtx, audioCodec, NULL) < 0){
        printf("Failed to open audio decoder");
        return 0;
    }

    AVPacket *pkt = av_packet_alloc();
    AVPacket *writePkt = av_packet_alloc();
    AVFrame *frame = nullptr;

    AVFrame *pFrameRGB = av_frame_alloc();
    int ret = 0;
    SwsContext* swsContext = sws_getContext(videoCodecCtx->width, videoCodecCtx->height, AV_PIX_FMT_YUV420P,videoCodecCtx->width, videoCodecCtx->height,
                                            AV_PIX_FMT_RGB24,NULL, NULL, NULL, NULL);
    int nb_frams = 0;
    while(av_read_frame(formatCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(videoCodecCtx, pkt) >= 0) {
                printf("decode video no. %d packet \n", pkt->pts);
                frame = av_frame_alloc();
                ret = avcodec_receive_frame(videoCodecCtx, frame);
                if (ret >= 0) {
                    printf("decode video %d frame \n", frame->pkt_pos);

                    global_width = frame->width;
                    global_height = frame->height;

                    AVFrame* rgbFrame = av_frame_alloc();
                    rgbFrame->format = AV_PIX_FMT_RGB24;
                    rgbFrame->width  = frame->width;
                    rgbFrame->height = frame->height;
                    av_image_alloc(rgbFrame->data, rgbFrame->linesize, frame->width, frame->height,
                                   AV_PIX_FMT_RGB24, 1);

                    int a = 3;
                    a++;
                    sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize);

                    mtx.lock();
                    framesQueue.emplace_back(rgbFrame);
                    mtx.unlock();

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

                av_free(frame);

            }
        }
         if (pkt->stream_index == audioStreamIndex) {
             nb_frams++;

             if (avcodec_send_packet(audioCodecCtx, pkt) >= 0) {
                 frame = av_frame_alloc();
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
                 av_free(frame);
             }
         }

//        av_packet_unref(pkt);
    }

    printf("number of frames:  %d  \n", nb_frams);
    avcodec_free_context(&audioCodecCtx);

    return 0;
}


int main() {
    framesQueue = std::deque<AVFrame*>();
    std::thread t2(decodeVideo);
    t2.detach();

    std::thread t1(renderVideo);
    t1.join();
    return 0;
}
