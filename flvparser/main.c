#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "io.h"
#include "flv.h"



uint32_t get_flv_timestamp(IOContext* ctx){
	uint32_t base_time = get_uint24(ctx);
	uint8_t extra_time = get_uint8(ctx);
	return extra_time << 24 | base_time;
}

int prase_tag_header(IOContext* ctx, TAG_HEADER* header)
{
	int ret = read(ctx, 11);
	if(ret < 11)
		return -1;

	header->type = get_uint8(ctx);
	header->data_size = get_uint24(ctx);
	header->timestamp = get_flv_timestamp(ctx);
	header->stream_id = get_uint24(ctx);
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
		TAG_HEADER tag_header;
		ret = prase_tag_header(ctx, &tag_header);
		if(ret != 0)
			break;
		printf("tag type = %d, size = %d\n", tag_header.type, tag_header.data_size);
		read_skip(ctx, tag_header.data_size);
		read_skip(ctx, 4);
	}


	return 0;
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