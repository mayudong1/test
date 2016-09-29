#ifndef __DEFINE__H_
#define __DEFINE__H_

typedef struct RTMP_Message
{
	int type;
	int payload_len;
	unsigned int timestamp;
	int streamid;
	char* body;
}RTMP_Message;

#endif