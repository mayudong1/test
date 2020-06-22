#include "io.h"
#include <stdlib.h>
#include <memory.h>

IOContext* open_input(const char* input){
	IOContext* ctx = (IOContext*)malloc(sizeof(IOContext));
	ctx->fd = fopen(input, "rb");
	if(ctx->fd == NULL){
		free(ctx);
		return NULL;
	}
	return ctx;
}

void close_input(IOContext* ctx){
	if(ctx == NULL)
		return;
	if(ctx->fd == NULL){
		memset(ctx, 0, sizeof(IOContext));
		free(ctx);
		return;
	}
	fclose(ctx->fd);
	free(ctx);
}

int read(IOContext* ctx, int size){
	int ret = fread(ctx->buffer, 1, size, ctx->fd);
	if(ret < 0)
		return ret;
	ctx->size = ret;
	ctx->cur_index = 0;
	return ret;
}


void read_skip(IOContext* ctx, int skip){
	if(ctx != NULL && ctx->fd != NULL)
		fseek(ctx->fd, skip, SEEK_CUR);
}


uint8_t get_uint8(IOContext* ctx){
	uint8_t ret = ctx->buffer[ctx->cur_index];
	ctx->cur_index++;
	return ret;
}

uint32_t get_uint24(IOContext* ctx){
	uint32_t ret = ctx->buffer[ctx->cur_index] << 16 | ctx->buffer[ctx->cur_index+1] << 8 | ctx->buffer[ctx->cur_index+2];
	ctx->cur_index += 3;
	return ret;
}
