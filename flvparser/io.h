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
uint16_t get_uint16(IOContext* ctx);
uint32_t get_uint24(IOContext* ctx);
uint32_t get_uint32(IOContext* ctx);
void get_skip(IOContext* ctx, int skip);

int get_data(IOContext* ctx, uint8_t* data, int size);
int peak_data(IOContext* ctx, uint8_t** data);


#endif