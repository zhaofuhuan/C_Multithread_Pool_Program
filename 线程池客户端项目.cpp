// 线程池客户端项目.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<signal.h>
#include<netdb.h>
#include<fcntl.h>
#include<pthread.h>
#define MAXLINE 4096
#define MAXN 1024

int connectTCP(const char*host, const char*service)
{
	struct hostent* phe;
	struct servent* pse;
	struct protoent* ppe;
	struct sockaddr_in servaddr;
	int s, type;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if (pse = getservbyname(service, "TCP"))
		servaddr.sin_port = pse->s_port;
	else if ((servaddr.sinport = htons((unsigned short)atoi(service))) == 0)
		printf("can't get \"%s\"service entry\n", service);
	if (phe = gethostbyname(host))
		memcpy(&servaddr.sin_addr, phe->h_addr, phe->h_length);
	else
		servaddr.sin_addr.s_addr = inet_addr(host);
	type = SOCK_STREAM;
	s = socket(AF_INET, type, 0);
	if (s < 0)
		printf("can't create socket:%s\n", strerror(errno));
	if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		printf("can't connect to%s,%s:%s\n", host, service, strerror(errno));
	return s;
}
int main(int argc,char** argv)
{
	int i, j, fd, nchildren, nloops, nbytes;
	pid_t pid;
	ssize_t n;
	char request[MAXLINE], reply[MAXN];
	if (argc != 6)
		printf("输入参数 数目 不为6");
	nchildren = atoi(argv[3]);
	nloops = atoi(argv[4]);
	nbytes = atoi(argv[5]);
	snprintf(request, sizeof(request), "%d\n", nbytes);
	for (i = 0; i < nchildren; i++)
	{
		if ((pid = fork()) == 0)
		{
			for (j = 0; j < nloops; j++)
			{
				fd = connectTCP(argv[1], argv[2]);
				send(fd, request, strlen(request), 0);
				if ((n = Readn(fd, reply, nbytes)) != nbytes)
					printf("server returned %d bytes", n);
				close(fd);
			}
			printf("child %d done\n", i);
			exit(0);
		}
	}
	while (wait(NULL) > 0)
	if (errno != ECHILD)
			printf("wait error!\n");
    exit(0);
}

ssize_t readn(int fd, void*vptr, size_t n) 
{
	size_t nleft;
	ssize_t nread;
	char* ptr;
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ((nread = recv(fd, ptr, nleft, 0)) < 0)
			if (erro == EINTR)
				nread = n;
			else
				return(-1);
		else if (nread == 0)
			break;
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);
}

ssize_t Readn(int fd, void*ptr, size_t nbytes)
{
	ssize_t n;
	if ((n = readn(fd, ptr, nbytes)) < 0)
		printf("readn error");
	return(n);
}
