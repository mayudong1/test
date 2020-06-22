#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "io.h"
#include "flv.h"

void show_tag_info(FLV_Tag* tag);


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
	tag->video.frame_type = get_uint8(ctx);
	tag->video.codec_id = get_uint8(ctx);
	tag->video.avcpacket_type = get_uint8(ctx);
	tag->video.cts = get_uint24(ctx);
	return 0;
}

int parse_video(IOContext* ctx, FLV_Tag* tag)
{
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
		ret = prase_tag_header(ctx, &tag);
		if(ret != 0)
			break;
		show_tag_info(&tag);
		parse_tag_body(ctx, &tag);
		read_skip(ctx, 4);
	}


	return 0;
}

void show_tag_info(FLV_Tag* tag)
{
	printf("type = %d, size = %d ", tag->type, tag->data_size);
	if(tag->type == TAG_TYPE_VIDEO){
		printf("frame_type = %d", tag->video.frame_type);
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