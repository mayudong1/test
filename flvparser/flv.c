#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#include "io.h"
#include "flv.h"
#include "amf.h"


void show_tag_info(FLV_Tag* tag);

void flv_tag_init(FLV_Tag* tag){
	memset(tag, 0, sizeof(FLV_Tag));
}


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

void get_amf_string(IOContext*ctx, char* str)
{
	int len = get_uint16(ctx);
	if(len > MAX_AMF_STR_LEN - 1){
		len = MAX_AMF_STR_LEN - 1;
	}
	int ret = get_data(ctx, (uint8_t*)str, len);
	str[ret] = 0;
}

double get_amf_number(IOContext* ctx){
	double dVal;
	uint8_t data[8];
	get_data(ctx, data, 8);
    unsigned char *ci, *co;
    ci = (unsigned char *)data;
    co = (unsigned char *)&dVal;
    co[0] = ci[7];
    co[1] = ci[6];
    co[2] = ci[5];
    co[3] = ci[4];
    co[4] = ci[3];
    co[5] = ci[2];
    co[6] = ci[1];
    co[7] = ci[0];
    return dVal;
}

bool get_amf_bool(IOContext* ctx){
	uint8_t data = get_uint8(ctx);
	return data != 0;
}


int parse_metadata(IOContext* ctx, FLV_Tag* tag)
{
	tag->metadata.amf0_type = get_uint8(ctx);
	get_amf_string(ctx, tag->metadata.amf0_data);
	tag->metadata.amf1_type = get_uint8(ctx);
	if(tag->metadata.amf1_type == AMF_ECMA_ARRAY){
		int count = get_uint32(ctx);
		if(count > MAX_METADATA_COUNT)
			count = MAX_METADATA_COUNT;
		tag->metadata.amf1_num = count;
		for(int i=0;i<count;i++){
			METADATA_INFO* meta = &tag->metadata.meta_array[i];
			get_amf_string(ctx, meta->key);
			meta->valueType = get_uint8(ctx);
			if(meta->valueType == AMF_NUMBER){
				meta->dValue = get_amf_number(ctx);
			} else if(meta->valueType == AMF_STRING){
				get_amf_string(ctx, meta->strValue);
			} else if(meta->valueType == AMF_BOOLEAN){
				meta->bValue = get_amf_bool(ctx);
			}
		}
	} else if(tag->metadata.amf1_type == AMF_OBJECT){
		int count = 0;
		while(1){
			METADATA_INFO* meta = &tag->metadata.meta_array[count];
			get_amf_string(ctx, meta->key);
			meta->valueType = get_uint8(ctx);
			if(meta->valueType == AMF_NUMBER){
				meta->dValue = get_amf_number(ctx);
			} else if(meta->valueType == AMF_STRING){
				get_amf_string(ctx, meta->strValue);
			} else if(meta->valueType == AMF_BOOLEAN){
				meta->bValue = get_amf_bool(ctx);
			} else if(meta->valueType == AMF_OBJECT_END){
				break;
			}
			count++;
			if(count >= MAX_METADATA_COUNT)
				break;
		}
		tag->metadata.amf1_num = count;
	}
	return 0;
}

int parse_h2645_nalu(IOContext* ctx, FLV_Tag* tag)
{
	int left_size = tag->data_size - 5;
	while(left_size > 0){
		if(left_size < 4)
			break;
		uint32_t len = get_uint32(ctx);
		left_size -= 4;
		if(len > left_size)
			break;
		int nalu_type = get_uint8(ctx);
		if(tag->video.codec_id == CODEC_H264){
			nalu_type = nalu_type & 0x1f;
		}else if(tag->video.codec_id == CODEC_H265){
			int nalu_type = (nalu_type & 0x7e) >> 1;
		}
		tag->video.nalu_list[tag->video.nalu_num++] = nalu_type;
		get_skip(ctx, len-1);
		left_size -= len;
		
	}
	return 0;
}

int parse_video(IOContext* ctx, FLV_Tag* tag)
{
	uint8_t tmp = get_uint8(ctx);
	tag->video.frame_type = tmp >> 4;
	tag->video.codec_id = tmp & 0x0f;
	tag->video.avcpacket_type = get_uint8(ctx);
	tag->video.cts = get_uint24(ctx);

	if(tag->video.avcpacket_type == SEQ_HEADER){
		int size = tag->data_size - 5;
		if(size > MAX_SEQ_HEADER_LEN){
			size = MAX_SEQ_HEADER_LEN;
		}
		int ret_size = get_data(ctx, tag->video.seq_header, size);
		tag->video.seq_header_len = ret_size;
	} else if(tag->video.avcpacket_type == NALU){
		parse_h2645_nalu(ctx, tag);
	}
	return 0;
}

int parse_audio(IOContext* ctx, FLV_Tag* tag)
{
	uint8_t tmp = get_uint8(ctx);
	tag->audio.format = tmp >> 4;
	tag->audio.sample_rate = (tmp >> 2) & 0x03;
	tag->audio.bit_width = (tmp >> 1) & 0x01;
	tag->audio.stereo = tmp & 0x01;
	tag->audio.aac_packet_type = -1;
	if(tag->audio.format == 10){
		tag->audio.aac_packet_type = get_uint8(ctx);
	}

	if(tag->audio.aac_packet_type == 0){
		tag->audio.seq_header_len = tag->data_size - 2;
		if(tag->audio.seq_header_len > MAX_SEQ_HEADER_LEN){
			tag->audio.seq_header_len = MAX_SEQ_HEADER_LEN;
		}
		get_data(ctx, tag->audio.seq_header, tag->audio.seq_header_len);
	}
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
		flv_tag_init(&tag);

		ret = prase_tag_header(ctx, &tag);
		if(ret != 0)
			break;

		parse_tag_body(ctx, &tag);
		read_skip(ctx, 4);
		show_tag_info(&tag);
	}

	return 0;
}

void show_tag_info(FLV_Tag* tag)
{
	printf("type = %d, size = %d ", tag->type, tag->data_size);
	if(tag->type == TAG_TYPE_VIDEO){
		printf("frame_type = %d(%x) ", tag->video.frame_type, tag->video.frame_type);
		if(tag->video.seq_header_len > 0){
			printf("sequence header: ");
			for(int i=0;i<tag->video.seq_header_len;i++){
				printf("%02x ", tag->video.seq_header[i]);
			}
		}
		if(tag->video.nalu_num > 0){
			printf("nalu list: ");
			for(int i=0;i<tag->video.nalu_num;i++){
				printf("%d ", tag->video.nalu_list[i]);
			}
		}
	} else if(tag->type == TAG_TYPE_METADATA){
		for(int i=0;i<tag->metadata.amf1_num;i++){
			printf("%s ", tag->metadata.meta_array[i].key);
		}
	} else if(tag->type == TAG_TYPE_AUDIO){
		if(tag->audio.seq_header_len > 0){
			printf("audio adts : ");
			for(int i=0;i<tag->audio.seq_header_len;i++){
				printf("%02x ", tag->audio.seq_header[i]);
			}
		}
	}
	printf("\n");
}
