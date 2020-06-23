#ifndef _FLV_DEFINE_H_
#define _FLV_DEFINE_H_
#include <stdbool.h>
#include "io.h"

#define MAX_SEQ_HEADER_LEN 1024
#define MAX_NALU_NUM 10

#define MAX_AMF_STR_LEN 128
#define MAX_METADATA_COUNT 64

#define TAG_TYPE_METADATA 0x12
#define TAG_TYPE_VIDEO 0x09
#define TAG_TYPE_AUDIO 0x08


enum VIDEO_CODEC{
	CODEC_H264 = 7,
	CODEC_HEVC = 12,
};

enum PACKET_TYPE{
	SEQ_HEADER = 0,
	NALU = 1,
	SEQ_END = 2,
};

typedef struct METADATA_INFO
{
    char key[MAX_AMF_STR_LEN];
    int valueType;
    bool bValue;
    double dValue;
    char strValue[MAX_AMF_STR_LEN];
}METADATA_INFO;

typedef struct TAG_METADATA{
	int amf0_type;
	char amf0_data[MAX_AMF_STR_LEN];
	int amf1_type;
	int amf1_num;
	METADATA_INFO meta_array[MAX_METADATA_COUNT];
}TAG_METADATA;

typedef struct TAG_VIDEO{
	int frame_type;
	int codec_id;
	int avcpacket_type;
	int cts;
	uint8_t seq_header[MAX_SEQ_HEADER_LEN];
	int seq_header_len;

	int nalu_num;
	int nalu_list[MAX_NALU_NUM];
}TAG_VIDEO;

typedef struct TAG_AUDIO{
	int format;
	int sample_rate;
	int bit_width;
	int stereo;
	int aac_packet_type;

	uint8_t seq_header[MAX_SEQ_HEADER_LEN];
	int seq_header_len;
}TAG_AUDIO;


typedef struct FLV_TAG{
	int type;
	int data_size;
	uint32_t timestamp;
	int stream_id;

	union{
		TAG_METADATA metadata;
		TAG_VIDEO video;
		TAG_AUDIO audio;
	};
}FLV_Tag;


int parse_flv(IOContext* ctx);

#endif