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
	if(ctx->fd != NULL){
		fclose(ctx->fd);
	}
	if(ctx->buffer != NULL){
		free(ctx->buffer);
	}
	free(ctx);
}

int read(IOContext* ctx, int size){
	if(size <= 0)
		return -1;

	if(ctx->buffer == NULL){
		ctx->buffer = (uint8_t*)malloc(size * sizeof(uint8_t));
		ctx->buffer_size = size;
	} else if(size > ctx->buffer_size){
		ctx->buffer = realloc(ctx->buffer, size);
		ctx->buffer_size = size;
	}

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

uint16_t get_uint16(IOContext* ctx){
	uint16_t ret = ctx->buffer[ctx->cur_index] << 8 | ctx->buffer[ctx->cur_index+1];
	ctx->cur_index += 2;
	return ret;
}

uint32_t get_uint24(IOContext* ctx){
	uint32_t ret = ctx->buffer[ctx->cur_index] << 16 | ctx->buffer[ctx->cur_index+1] << 8 | ctx->buffer[ctx->cur_index+2];
	ctx->cur_index += 3;
	return ret;
}

uint32_t get_uint32(IOContext* ctx){
	uint32_t ret = ctx->buffer[ctx->cur_index] << 24 | ctx->buffer[ctx->cur_index+1] << 16 | ctx->buffer[ctx->cur_index+2] << 8 | ctx->buffer[ctx->cur_index+3];
	ctx->cur_index += 4;
	return ret;
}

void get_skip(IOContext* ctx, int skip){
	ctx->cur_index += skip;
}

int get_data(IOContext* ctx, uint8_t* data, int size){
	int left_size = ctx->size - ctx->cur_index;
	if(size > left_size){
		size = left_size;
		if(size <= 0)
			return 0;
	}
	memcpy(data, &ctx->buffer[ctx->cur_index], size);
	ctx->cur_index += size;
	return size;
}

int peak_data(IOContext* ctx, uint8_t** data){
	*data = &ctx->buffer[ctx->cur_index];
	return ctx->size - ctx->cur_index;
}