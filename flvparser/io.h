#ifndef _FLV_IO_H_
#define _FLV_IO_H_

#include <stdio.h>
#include <stdint.h>

typedef struct IOContext{
	FILE* fd;
	uint8_t* buffer;
	int buffer_size;
	int size;
	int cur_index;
}IOContext;

IOContext* open_input(const char* input);
void close_input(IOContext* ctx);

int read(IOContext* ctx, int size);
void read_skip(IOContext* ctx, int skip);

uint8_t get_uint8(IOContext* ctx);
uint32_t get_uint24(IOContext* ctx);



#endif