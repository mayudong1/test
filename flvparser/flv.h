#ifndef _FLV_DEFINE_H_
#define _FLV_DEFINE_H_

typedef struct TAG_HEADER{
	int type;
	int data_size;
	uint32_t timestamp;
	int stream_id;
}TAG_HEADER;


#endif