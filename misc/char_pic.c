#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"

const char* g_file_name = "/Users/mayudong/Downloads/a.jpeg";
const int target_widht = 100;
const int target_height = 50;

void PrintPic(uint8_t* data, int width, int height){
    char ch[] = { ' ', '`', '.', '^', ',', ':', '~', '"', '<', '!', 'c', 't', '+','{', 'i', '7', '?', 'u', '3', '0', 'p', 'w', '4', 'A', '8', 'D', 'X', '%', '#', 'H', 'W', 'M' };
    int count = sizeof(ch)/sizeof(ch[0]);

    for(int i=0;i<height;i++){
        for(int j=0;j<width;j++){
            int gray = data[i*width+j];
            int index = gray*count/255;
            index = count - index;
            if(index < 0)
                index = 0;
            if(index >= count)
                index = count-1;
            printf("%c", ch[index]);
        }
        printf("\n");
    }
}

int main(int argc, char** argv){
    char file_name[2048];
    if(argc == 1){
        strncpy(file_name, g_file_name, 2048);
    }else{
        strncpy(file_name, argv[1], 2048);
        file_name[2047] = 0;
    }


    av_log_set_level(AV_LOG_QUIET);
    AVFormatContext* ctx = NULL;
    int ret = avformat_open_input(&ctx, file_name, 0, NULL);
    if(ret != 0){
        printf("open failed\n");
        return -1;
    }
    
    ret = avformat_find_stream_info(ctx, NULL);
    if(ret != 0){
        printf("find stream info failed\n");
        return -1;
    }

    if(ctx->nb_streams < 1){
        printf("no streams\n");
        return -1;
    }

    AVStream *st = ctx->streams[0];
    if(st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO){
        printf("no pic\n");
        return -1;
    }

    AVCodec *dec = avcodec_find_decoder(st->codecpar->codec_id);
    if(dec == NULL){
        printf("no decoder\n");
        return -1;
    }

    AVCodecContext* dec_ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(dec_ctx, st->codecpar);
    ret = avcodec_open2(dec_ctx, dec, NULL);
    if(ret != 0){
        printf("open codec failed\n");
    }

    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    while(1){
        ret = av_read_frame(ctx, packet);
        ret = avcodec_send_packet(dec_ctx, packet);
        ret = avcodec_receive_frame(dec_ctx, frame);
        if(ret >= 0){
            break;
        }
    }
    avcodec_free_context(&dec_ctx);

    av_packet_free(&packet);

    struct SwsContext *sws_ctx;
    sws_ctx = sws_getContext(frame->width, frame->height, frame->format,
                             target_widht, target_height, AV_PIX_FMT_GRAY8,
                             SWS_BILINEAR, NULL, NULL, NULL);

    uint8_t* dst_data[4];
    int dst_linesize[4];
    av_image_alloc(dst_data, dst_linesize, target_widht, target_height, AV_PIX_FMT_GRAY8, 1);

    sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, dst_data, dst_linesize);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    
    PrintPic(dst_data[0], target_widht, target_height);

    return 0;
}