#include "ts.h"
#include <stdbool.h>

#define TS_PACKET_LEN 188

typedef struct TS_Header{
	uint8_t sync_byte;
	bool transport_error_indicator;
	bool payload_uint_start_indicator;
	bool transport_priority;
	uint16_t pid;
	uint8_t transport_scrambling_control;
	uint8_t adaptation_field_control;
	uint8_t continutiy_counter;
}TS_Header;


static void show_ts_header(TS_Header* header){
	printf("pid = 0x%x, payload_start = %d, adaptation = %d, counter = %d\n", 
		header->pid, header->payload_uint_start_indicator,
		header->adaptation_field_control, header->continutiy_counter);
}

int parse_ts(IOContext* ctx)
{
	while(1){
		int ret = read(ctx, TS_PACKET_LEN);
		if(ret < 188)
			break;
		TS_Header header;

		uint8_t sync_byte = get_uint8(ctx);
		if(sync_byte != 0x47){
			printf("sync word is not 0x47");
			break;
		}
		header.sync_byte = sync_byte;

		uint16_t tmp = get_uint16(ctx);
		header.transport_error_indicator = (tmp >> 15) & 0x01;
		header.payload_uint_start_indicator = (tmp >> 14) & 0x01;
		header.pid = tmp & 0x1fff;

		uint8_t tmp2 = get_uint8(ctx);
		header.transport_scrambling_control = (tmp2 >> 6);
		header.adaptation_field_control = (tmp2 >> 4) & 0x03;
		header.continutiy_counter = tmp2 & 0x0f;
		
		show_ts_header(&header);
	}
	
	return 0;
}