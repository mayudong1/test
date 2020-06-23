#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#include "io.h"
#include "flv.h"

void show_tag_info(FLV_Tag* tag);

void flv_tag_init(FLV_Tag* tag){
	memset(tag, 0, sizeof(FLV_Tag));
}


uint32_t get_flv_timestamp(IOContext* ctx){
	uint32_t base_time = get_uint24(ctx);
	uint8_t extra_time = get_uint8(ctx);
	return extra_time << 24 | base_time;
}

int prase_tag_header(IOContext* ctx, FLV_Tag* tag)
{
	int ret = read(ctx, 11);
	if(ret < 11)
		return -1;

	tag->type = get_uint8(ctx);
	tag->data_size = get_uint24(ctx);
	tag->timestamp = get_flv_timestamp(ctx);
	tag->stream_id = get_uint24(ctx);
	return 0;
}

int parse_metadata(IOContext* ctx, FLV_Tag* tag)
{
	
	return 0;
}

int parse_h264_nalu(IOContext* ctx, FLV_Tag* tag)
{
	int left_size = tag->data_size - 5;
	while(left_size > 0){
		if(left_size < 4)
			break;
		uint32_t len = get_uint32(ctx);
		left_size -= 4;
		if(len > left_size)
			break;
		int nalu_type = get_uint8(ctx) & 0x1f;
		tag->video.nalu_list[tag->video.nalu_num++] = nalu_type;
		get_skip(ctx, len-1);
		left_size -= len;
		
	}
	return 0;
}

int parse_h265_nalu(IOContext* ctx, FLV_Tag* tag)
{
	return 0;
}

int parse_video(IOContext* ctx, FLV_Tag* tag)
{
	uint8_t tmp = get_uint8(ctx);
	tag->video.frame_type = tmp >> 4;
	tag->video.codec_id = tmp & 0x0f;
	tag->video.avcpacket_type = get_uint8(ctx);
	tag->video.cts = get_uint24(ctx);

	if(tag->video.avcpacket_type == SEQ_HEADER){
		int size = tag->data_size - 5;
		if(size > MAX_SEQ_HEADER_LEN){
			size = MAX_SEQ_HEADER_LEN;
		}
		int ret_size = get_data(ctx, tag->video.seq_header, size);
		tag->video.seq_header_len = ret_size;
	} else if(tag->video.avcpacket_type == NALU){
		if(tag->video.codec_id == CODEC_H264){
			parse_h264_nalu(ctx, tag);
		} else if(tag->video.codec_id == CODEC_H265){
			parse_h265_nalu(ctx, tag);
		}
	}
	return 0;
}

int parse_audio(IOContext* ctx, FLV_Tag* tag)
{
	return 0;
}

int parse_tag_body(IOContext* ctx, FLV_Tag* tag)
{
	int ret = read(ctx, tag->data_size);
	if(ret < tag->data_size)
		return -1;

	switch(tag->type){
	case TAG_TYPE_METADATA:
		parse_metadata(ctx, tag);
		break;
	case TAG_TYPE_VIDEO:
		parse_video(ctx, tag);
		break;
	case TAG_TYPE_AUDIO:
		parse_audio(ctx, tag);
		break;
	default:
		break;
	}
	return 0;
}


int parse_flv(IOContext* ctx){
	int ret = read(ctx, 13);
	if(ret < 13){
		printf("input file is too small.\n");
		return -1;
	}
	if(!(ctx->buffer[0] == 'F' && ctx->buffer[1] == 'L' && ctx->buffer[2] == 'V')){
		printf("input file is not flv format.\n");
		return -1;
	}

	while(1){
		FLV_Tag tag;
		flv_tag_init(&tag);

		ret = prase_tag_header(ctx, &tag);
		if(ret != 0)
			break;

		parse_tag_body(ctx, &tag);
		read_skip(ctx, 4);
		show_tag_info(&tag);
	}


	return 0;
}

void show_tag_info(FLV_Tag* tag)
{
	printf("type = %d, size = %d ", tag->type, tag->data_size);
	if(tag->type == TAG_TYPE_VIDEO){
		printf("frame_type = %d(%x) ", tag->video.frame_type, tag->video.frame_type);
		if(tag->video.seq_header_len > 0){
			printf("sequence header: ");
			for(int i=0;i<tag->video.seq_header_len;i++){
				printf("%02x ", tag->video.seq_header[i]);
			}
		}
		if(tag->video.nalu_num > 0){
			printf("nalu list: ");
			for(int i=0;i<tag->video.nalu_num;i++){
				printf("%02x ", tag->video.nalu_list[i]);
			}
		}
	}
	printf("\n");
}

int main(int argc, char** argv)
{
	if(argc < 2){
		printf("no input file.\n");
		return 0;
	}

	const char* input = argv[1];
	IOContext* ctx = open_input(input);
	if(ctx == NULL){
		printf("open input file failed. filename = %s\n", input);
		return 0;
	}

	parse_flv(ctx);

	close_input(ctx);

	return 0;
}