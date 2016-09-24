#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define HANDSHAKE_LEN 1536


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

int main(int argc,char *argv[])  
{  
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
   
    close(sockfd);

    return 0;
}  