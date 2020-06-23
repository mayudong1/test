#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#include "io.h"
#include "flv.h"
#include "amf.h"


static void show_tag_info(FLV_Tag* tag);

static void flv_tag_init(FLV_Tag* tag){
	memset(tag, 0, sizeof(FLV_Tag));
}


static uint32_t get_flv_timestamp(IOContext* ctx){
	uint32_t base_time = get_uint24(ctx);
	uint8_t extra_time = get_uint8(ctx);
	return extra_time << 24 | base_time;
}

static void get_amf_string(IOContext*ctx, char* str)
{
	int len = get_uint16(ctx);
	if(len > MAX_AMF_STR_LEN - 1){
		len = MAX_AMF_STR_LEN - 1;
	}
	int ret = get_data(ctx, (uint8_t*)str, len);
	str[ret] = 0;
}

static double get_amf_number(IOContext* ctx){
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

static bool get_amf_bool(IOContext* ctx){
	uint8_t data = get_uint8(ctx);
	return data != 0;
}

static int prase_tag_header(IOContext* ctx, FLV_Tag* tag)
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

static int parse_metadata(IOContext* ctx, FLV_Tag* tag)
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

static bool need_record(int codec_id, int nalu_type){
	if(codec_id == CODEC_H264){
		if(nalu_type == H264_NALU_SEI || nalu_type == H264_NALU_SPS || nalu_type == H264_NALU_PPS)
			return true;
	}else{
		if(nalu_type == HEVC_NALU_VPS 
			|| nalu_type == HEVC_NALU_SPS
			|| nalu_type == HEVC_NALU_PPS
			|| nalu_type == HEVC_NALU_PREFIX_SEI
			|| nalu_type == HEVC_NALU_SUFFIX_SEI)
			return true;
	}
	return false;
}
static int parse_h2645_nalu(IOContext* ctx, FLV_Tag* tag)
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
			if(need_record(tag->video.codec_id, nalu_type) && tag->video.encode_param_num < MAX_ENCODE_PARAM_COUNT){
				ENCODE_PARAM_INFO* param = &tag->video.encode_param[tag->video.encode_param_num++];
				int nalu_len = len-1;
				if(nalu_len > MAX_ENCODE_PARAM_LEN)
					nalu_len = MAX_ENCODE_PARAM_LEN;
				if(nalu_type == H264_NALU_SEI){
					strcpy(param->name, "SEI");
				} else if(nalu_type == H264_NALU_SPS){
					strcpy(param->name, "SPS");
				} else if(nalu_type == H264_NALU_PPS){
					strcpy(param->name, "PPS");
				}
				param->data_len = nalu_len;
				int ret = get_data(ctx, param->data, param->data_len);
				get_skip(ctx, len-1-ret);
			}else{
				get_skip(ctx, len-1);
			}
		}else if(tag->video.codec_id == CODEC_HEVC){
			nalu_type = ((nalu_type & 0x7e) >> 1);
			if(need_record(tag->video.codec_id, nalu_type) && tag->video.encode_param_num < MAX_ENCODE_PARAM_COUNT){
				get_skip(ctx, 1);
				ENCODE_PARAM_INFO* param = &tag->video.encode_param[tag->video.encode_param_num++];
				int nalu_len = len-2;
				if(nalu_len > MAX_ENCODE_PARAM_LEN)
					nalu_len = MAX_ENCODE_PARAM_LEN;
				if(nalu_type == HEVC_NALU_VPS){
					strcpy(param->name, "VPS");
				} else if(nalu_type == HEVC_NALU_SPS){
					strcpy(param->name, "SPS");
				} else if(nalu_type == HEVC_NALU_PPS){
					strcpy(param->name, "PPS");
				} else if(nalu_type == HEVC_NALU_PREFIX_SEI){
					strcpy(param->name, "PREFIX_SEI");
				} else if(nalu_type == HEVC_NALU_SUFFIX_SEI){
					strcpy(param->name, "SUFFIX_SEI");
				}
				param->data_len = nalu_len;
				int ret = get_data(ctx, param->data, param->data_len);
				get_skip(ctx, len-2-ret);
			}else{
				get_skip(ctx, len-1);	
			}
		}
		tag->video.nalu_list[tag->video.nalu_num++] = nalu_type;
		left_size -= len;
		
	}
	return 0;
}

static int parse_video(IOContext* ctx, FLV_Tag* tag)
{
	uint8_t tmp = get_uint8(ctx);
	tag->video.frame_type = tmp >> 4;
	tag->video.codec_id = tmp & 0x0f;
	if(tag->video.codec_id == CODEC_H264 || tag->video.codec_id == CODEC_HEVC){
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
	}

	
	return 0;
}

static int parse_audio(IOContext* ctx, FLV_Tag* tag)
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

static int parse_tag_body(IOContext* ctx, FLV_Tag* tag)
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

static void show_metadata(FLV_Tag* tag)
{
	printf("Metadata Packet, size = %d, metadata count = %d\n", tag->data_size, tag->metadata.amf1_num);
	for(int i=0;i<tag->metadata.amf1_num;i++){
		METADATA_INFO* meta = &tag->metadata.meta_array[i];
		printf("\t%s : ", meta->key);
		if(meta->valueType == AMF_NUMBER){
			printf("%f\n", meta->dValue);
		} else if(meta->valueType == AMF_BOOLEAN){
			printf("%d\n", meta->bValue);
		} else if(meta->valueType == AMF_STRING){
			printf("%s\n", meta->strValue);
		}
	}
}

static char* H264_NALU_NAME[] = {
	"Unkown", 	//0
	"P/B",		//1
	"P/B",		//2
	"P/B",		//3
	"P/B",		//4
	"I",		//5
	"SEI",		//6
	"SPS",		//7
	"PPS",		//8
	"AUD",		//9
	"EOS",		//10
	"EOB",		//11
};

static char* HEVC_NALU_NAME[] = {
	"TRAIL_N", //0
	"TRAIL_R", //1
	"TSA_N", //2
	"TSA_R", //3
	"STSA_N", //4
	"STSA_R", //5
	"RADL_N", //6
	"RADL_R", //7
	"RASL_N", //8
	"RASL_R", //9
	"RSV_VCL_N10", //10
	"RSV_VCL_R11", //11
	"RSV_VCL_N12", //12
	"RSV_VCL_R13", //13
	"RSV_VCL_N14", //14
	"RSV_VCL_R15", //15
	"BLA_W_LP", //16
	"BLA_W_RADL", //17
	"BLA_N_LP", //18
	"IDR_W_RADL", //19
	"IDR_N_LP", //20
	"CRA_NUT", //21
	"RSV_IRAP_VCL22", //22
	"RSV_IRAP_VCL23", //23
	"RSV_VCL24", //24
	"RSV_VCL25", //25
	"RSV_VCL26", //26
	"RSV_VCL27", //27
	"RSV_VCL28", //28
	"RSV_VCL29", //29
	"RSV_VCL30", //30
	"RSV_VCL31", //31
	"VPS_NUT", //32
	"SPS_NUT", //33
	"PPS_NUT", //34
	"AUD_NUT", //35
	"EOS_NUT", //36
	"EOB_NUT", //37
	"FD_NUT", //38
	"PREFIX_SEI_NUT", //39
	"SUFFIX_SEI_NUT", //40
};

static char* get_nalu_name(int codec_id, int nalu_type){
	if(codec_id == CODEC_H264){
		if(nalu_type > sizeof(H264_NALU_NAME) / sizeof(H264_NALU_NAME[0]))
			return "Unkown";
		else
			return H264_NALU_NAME[nalu_type];
	} else if(codec_id == CODEC_HEVC){
		if(nalu_type > sizeof(HEVC_NALU_NAME) / sizeof(HEVC_NALU_NAME[0]))
			return "Unkown";
		else
			return HEVC_NALU_NAME[nalu_type];
	}
	return "Unkown";
}

static char* get_video_codec(int id)
{
	switch(id){
	case 2:
		return "Sorenson H.263";
	case 3:
		return "Screen video";
	case 4:
		return "On2 VP6";
	case 5:
		return "VP6";
	case 6:
		return "Screen video version 2";
	case 7:
		return "H264";
	case 12:
		return "HEVC";
	default:
		return "Unkown";
	}
}

static void show_video(FLV_Tag* tag)
{
	static int count = 0;
	printf("Video Packet %d, size=%d, ", count++, tag->data_size);
	printf("stream_id=%d, ", tag->stream_id);
	printf("codec=%s, ", get_video_codec(tag->video.codec_id));
	printf("key_frame=%d, ", (tag->video.frame_type == 1));
	printf("dts=%d, pts=%d, ", tag->timestamp, tag->timestamp + tag->video.cts);

	if(tag->video.codec_id == CODEC_H264 || tag->video.codec_id == CODEC_HEVC){
		if(tag->video.avcpacket_type == SEQ_HEADER){
			printf("sequence header=");
			for(int i=0;i<tag->video.seq_header_len;i++){
				printf("%02x", tag->video.seq_header[i]);
			}
		}else if(tag->video.avcpacket_type == SEQ_END){
			printf("sequence end.");
		}else if(tag->video.avcpacket_type == NALU){
			printf("Nalu:");
			for(int i=0;i<tag->video.nalu_num;i++){
				printf("%s(%d) ", get_nalu_name(tag->video.codec_id, tag->video.nalu_list[i]), tag->video.nalu_list[i]);
			}
		}
	}
	printf("\n");
	for(int i=0;i<tag->video.encode_param_num;i++){
		printf("%s:", tag->video.encode_param[i].name);
		for(int j=0;j<tag->video.encode_param[i].data_len;j++){
			printf("%02x ", tag->video.encode_param[i].data[j]);
		}
		printf("\n");
	}
}


static char* AUDIO_FORMAT[] = {
	"PCM", //0
	"ADPCM", //1
	"MP3", //2
	"PCM", //3
	"Nellymoser", //4
	"Nellymoser", //5
	"Nellymoser", //6
	"PCM", //7
	"PCM", //8
	"", //9
	"AAC", //10
	"Speex", //11
	"", //12
	"", //13
	"MP3", //14
	"Device-specific", //15
};

static char* get_audio_codec(int id)
{
	if(id > sizeof(AUDIO_FORMAT)/sizeof(AUDIO_FORMAT[0]))
		return "Unkown";
	else
		return AUDIO_FORMAT[id];
}

static char* get_audio_sample_rate(int id)
{
	switch(id){
	case 0:
		return "5.5kHz";
	case 1:
		return "11kHz";
	case 2:
		return "22kHz";
	case 3:
		return "44kHz";
	default:
		return "";
	}
}

static char* get_audio_bit_width(int id)
{
	if(id == 0)
		return "8bit";
	else
		return "16bit";
}

static char* get_audio_channel(int id)
{
	if(id == 0)
		return "Mono";
	else
		return "Stereo";
}
static void show_audio(FLV_Tag* tag)
{
	static int count = 0;
	printf("Audio Packet %d, size=%d, ", count++, tag->data_size);
	printf("stream_id=%d, ", tag->stream_id);
	printf("codec=%s(%s %s %s), ", 
		get_audio_codec(tag->audio.format), 
		get_audio_sample_rate(tag->audio.sample_rate),
		get_audio_bit_width(tag->audio.bit_width),
		get_audio_channel(tag->audio.stereo));
	printf("dts=%d, ", tag->timestamp);
	if(tag->audio.format == 10 && tag->audio.aac_packet_type == 0){
		printf("sequence header=\"");
		for(int i=0;i<tag->audio.seq_header_len;i++){
			printf("%02x ", tag->audio.seq_header[i]);
		}
		printf("\b\"");
	}
	printf("\n");
}

static void show_unkown(FLV_Tag* tag)
{
	printf("Unkown Packet, size = %d, ", tag->data_size);	
	printf("stream_id=%d, ", tag->stream_id);
	printf("\n");
}

static void show_tag_info(FLV_Tag* tag)
{
	if(tag->type == TAG_TYPE_VIDEO){
		show_video(tag);
	} else if(tag->type == TAG_TYPE_AUDIO){
		show_audio(tag);
	} else if(tag->type == TAG_TYPE_METADATA){
		show_metadata(tag);
	} else{
		show_unkown(tag);
	}
}
