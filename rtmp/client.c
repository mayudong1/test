#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#include "define.h"
#include "amf.h"
#include "helper.h"

#define HANDSHAKE_LEN 1536

static int chunck_size = 128;


int my_recv(int sockfd, char* buf, int len)
{
	int recved = 0;
	int remain = len;
	while(1)
	{
		int ret = recv(sockfd, buf+recved, remain, 0);
		if(ret == -1)
		{
			return -1;
		}
		recved += ret;
		remain -= ret;
		if(remain == 0)
		{
			break;
		}
	}
	return len;
}

int handshake(int sockfd)
{
 	char clientbuf[HANDSHAKE_LEN + 1];    

    clientbuf[0] = 3;
    clientbuf[1] = 0;
    clientbuf[2] = 0;
    clientbuf[3] = 0;
    clientbuf[4] = 0;

    int sended = send(sockfd, clientbuf, HANDSHAKE_LEN + 1, 0);
    if(sended == -1)
    {
    	printf("send hand shake data failed\n");
    	return -1;
    }
    else
    {
    	printf("send s0 s1 success.\n");
    }

    char s0;
	char serversig[HANDSHAKE_LEN];
	int recved = 0;

	recved = recv(sockfd, &s0, 1, 0);
	if(recved != 1)
	{
		printf("recv s0 failed\n");
		return -1;
	}
	else
	{
		printf("recv s0 = %d success.\n", s0);
	}

    recved = my_recv(sockfd, serversig, HANDSHAKE_LEN);
    if(recved != HANDSHAKE_LEN)
    {
    	printf("recv s1 failed, recved = %d\n", recved);
    	return -1;
    }
    else
    {
    	printf("recv s1 success.\n");
    }

    sended = send(sockfd, serversig, HANDSHAKE_LEN, 0);
    if(sended == -1)
    {
    	printf("send s2 failed\n");
    	return -1;
    }

    recved = my_recv(sockfd, serversig, HANDSHAKE_LEN);
    if(recved != HANDSHAKE_LEN)
    {
    	printf("recv s2 failed, recved = %d\n", recved);
    	return -1;
    }
    else
    {
    	printf("recv s2 success.\n");    	
    }
    return 0;
}

int send_message(int sockfd, RTMP_Message* msg)
{
    // print_hex(msg->body, msg->payload_len);

    char header[256];
    memset(header, 0, 256);
    char* basic_header = header;    
    char* msg_header = &header[1];
    int header_len = 0;

    int chunck_count = (msg->payload_len / chunck_size) + ((msg->payload_len % chunck_size) == 0 ? 0 : 1);
    int remain = msg->payload_len;
    for(int i=0;i<chunck_count;i++)
    {
        if(i == 0)
        {
            *basic_header = 0x00;
            *basic_header |= msg->channel;

            msg_header += 3;//timestamp
            msg_header += encode_int24_be(msg_header, msg->payload_len);
            *msg_header++ = msg->type;
            msg_header += encode_int32_le(msg_header, msg->streamid);            
            header_len = msg_header - header;
        }
        else
        {
            *basic_header = 0xc0;
            *basic_header |= msg->channel;
            header_len = 1;
        }
                
        send(sockfd, header, header_len, 0);
        // print_hex(header, header_len);
        
        send(sockfd, msg->body + (i * chunck_size), MIN(remain, chunck_size), 0);
        // print_hex(msg->body + (i * chunck_size), MIN(remain, chunck_size));

        remain -= chunck_size;
    }

    return 0;
}

int send_connect_msg(int sockfd, RTMP_Message* _msg)
{
    if(_msg)
    {
        return send_message(sockfd, _msg);        
    }

    RTMP_Message msg;
    char buffer[4096] = {0};
    char* enc = buffer;

    enc = encode_string(enc, "connect");
    enc = encode_number(enc, 1);

    *enc++ = AMF_OBJECT;
    enc = encode_named_string(enc, "app", "myApp");
    enc = encode_named_string(enc, "flashVer", "testVer");
    // enc = encode_named_string(enc, "swfUrl", "FlvPlayer.swf");
    enc = encode_named_string(enc, "tcUrl", "rtmp://live.hkstv.hk.lxdns.com:1935/live");
    enc = encode_named_bool(enc, "fpad", 0);
    enc = encode_named_number(enc, "capabilities", 15);
    enc = encode_named_number(enc, "audioCodecs", 4071); //aac
    enc = encode_named_number(enc, "videoCodecs", 252); //h264
    enc = encode_named_number(enc, "videoFunction", 1.0);
    enc = encode_named_string(enc, "pageUrl", "page/url");
    enc = encode_named_number(enc, "objectEncodeing", 3);//amf3

    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;
    
    msg.type = 20;
    msg.payload_len = enc - buffer;
    msg.timestamp = 0;
    msg.streamid = 0;
    msg.channel = 3;
    msg.body = buffer;

    send_message(sockfd, &msg);
    return 0;
}

void test()
{
    printf("this is in test\n");
}


int main(int argc,char *argv[])  
{  
    if(argc > 1)
    {
        test();
        return 0;
    }

	char* server_name = "live.hkstv.hk.lxdns.com";
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_name);
    if(server.sin_addr.s_addr == INADDR_NONE)
    {
    	struct hostent *host = gethostbyname(server_name);
    	if(host == NULL || host->h_addr == NULL)
    	{
    		printf("gethostbyname failed\n");
    		return -1;
    	}
    	server.sin_addr = *(struct in_addr*)host->h_addr;
    }
    server.sin_port = htons(1935);

    int ret = connect(sockfd, (struct sockaddr*)&server, sizeof(server));
    if(ret == -1)
    {
    	printf("connect failed\n");
    	return -1;
    }
    else
    {
    	printf("connect success.\n");
    }

    ret = handshake(sockfd);
    if(ret != 0)
    {
    	printf("handshake failed\n");
    	return -1;
    }
    else
    {
    	printf("handshake success.\n");
    }
   
    send_connect_msg(sockfd, NULL);
    char aaa[1024];
    ret = recv(sockfd, aaa, 1024, 0);
    if(ret > 0)
    {
        printf("---------\n");
        print_hex(aaa, ret);    
    }
    else
    {
        printf("recv failed， ret = %d\n", ret);
    }

    close(sockfd);

    return 0;
}  