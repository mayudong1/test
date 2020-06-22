#ifndef _FLV_DEFINE_H_
#define _FLV_DEFINE_H_

#define TAG_TYPE_METADATA 0x12
#define TAG_TYPE_VIDEO 0x09
#define TAG_TYPE_AUDIO 0x08


typedef struct TAG_VIDEO{
	int frame_type;
	int codec_id;
	int avcpacket_type;
	int cts;
}TAG_VIDEO;


typedef struct FLV_TAG{
	int type;
	int data_size;
	uint32_t timestamp;
	int stream_id;

	union{
		TAG_VIDEO video;
	};
}FLV_Tag;



#endif