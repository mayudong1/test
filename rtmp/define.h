#ifndef __DEFINE__H_
#define __DEFINE__H_

#define MIN(a, b) ((a < b)?(a):(b))

typedef struct RTMP_Message
{	
	int type;
	int payload_len;
	unsigned int timestamp;
	int streamid;	
	int channel;
	char* body;
}RTMP_Message;

#endif