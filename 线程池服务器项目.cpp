// 线程池服务器项目.cpp : 定义控制台应用程序的入口点。
//

#include<unistd.h>
#include<syslog.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<signal.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netinet/tcp.h>
#include<sys/resource.h>
#include<fcntl.h>
#include<pthread.h>
#define PORT 10000 //定义端口号 10000
#define LISTENQ 1024 //定义等待队列长度1024
#define MAXLINE 4096 //定义数据缓存区大小4M
#define MAXN 1024 //定义服务器端发送的最大字节数1024
typedef struct { //定义线程池结构体
	pthread_t thread_tid; //线程ID
	long thread_count; //线程处理的客户连接数量
}Thread;
Thread* ptr; //声明线程池结构体变量
#define  MAXNCLL 32 //定义可打开的从套接字最大个数为32
int clifd[MAXNCLL], iget, iput; //clifd数组用于主线程往其中存入已接受的已连接套接字描述符，并由线程池中的可用线程从中取出一个服务
//响应客户，iput是往数组里存入的下一个元素下标，iget是从数组中取出下一个元素的下标
pthread_mutex_t clifd_mutex = PTHREAD_MUTEX_INITIALIZER;//初始化互斥锁变量
pthread_cond_t clifd_cond = PTHREAD_COND_INITIALIZER;	//初始化条件变量
int msock, nthreads;	//声明主线程描述符变量和线程个数变量nthreads
socklen_t addrlen;	//声明端点地址结构长度变量
struct sockaddr_in* clientaddr;	//声明客户端套接字端点地址结构体变量
static pid_t* pids;//声明线程描述符变量
void web_child(int);	//声明事务处理函数
void sig_int(int);	//声明信号处理函数
static pthread_mutex_t* mptr;	//声明互斥锁变量
ssize_t read_fd(int, void*, size_t, int*);	//声明read_fd函数
ssize_t write_fd(int, void*, size_t, int);	//声明write_fd函数
void* thread_main(void*)	//声明线程体执行函数
void thread_make(int i);	//声明线程池中从线程创建函数

int main(int argc,char**argv)
{
	int i, navail, maxfd, nsel, rc;
	int ssock;
	pthread_t tid;	//声明线程描述符变量
	socklen_t len;	//端点地址结构长度变量
	struct sockaddr_in servaddr;	//服务器套接字端点地址结构变量
	const int on = 1;
	ssize_t n;
	fd_set rset,masterset;
    
	msock = socket(AF_INET, SOCK_STREAM, 0);//创建主套接字 
	meset(&servaddr, 0, sizeof(servaddr));
	//服务器套接字地址变量赋值、
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);
	//设置主套接字属性
	setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bind(msock, (struct sockaddr*)&srvaddr, sizeof(servaddr));
	listen(msock, LISTENQ);
	
	nthreads = 10;//初始化线程池中的线程个数为10
	ptr = (Thread*)calloc(nthreads, sizeof(Thread));//为线程池结构体变量分配内存
	iget = iput = 0;
	for (i = 0; i < nthreads; i++)
		thread_make(i);//预先生成10个从线程
	signal(SIGINT, sig_int);//信号处理函数 sig_int为执行函数 按下信号SIGINT (CTRL+O)  
	for (;;) 
	{
		len = sizeof(clientaddr);
		ssock = accept(msock, clientaddr, &len);
		//操作共享变量clifd,iput等，先互斥锁上锁
		pthread_mutex_lock(&clifd_mutex); 
		clifd[iput] = ssock;

		if (++iput == MAXNCLL) {
			iput = 0;
		}
		if (++iput == iget) {
			printf("iput=iget=%d\n", iput);
			exit(-1);
		}
		pthread_cond_signal(&clifd_cond);//该函数向条件变量发送信号 唤醒阻塞在其上的线程
		pthread_mutex_unlock(&clifd_mutex);//共享变量操作完毕 互斥锁解锁
	}
}

void thread_make(int i) {//为线程池创建新的线程
	void*thread_main(void*);
	pthread_create(&ptr[i].thread_tid, NULL, &thread_main, (void)i);
	return;
}

void*thread_main(void* arg)
{//线程执行体函数
	int connfd; //套接字描述符局部变量
	void web_child(int);
	printf("thread %d starting \n", (int)arg);
	for (;;) 
	{
		//操作共享变量iput,iget,clifd之前，先对互斥锁上锁
		pthread_mutex_lock(&clifd_mutex);
		printf("get lock,thread=[%d]\n", (int)arg);
		while (iget == iput)//所有活动的套接字均已处理完毕
			pthread_cond_wait(&clifd_cond, &clifd_mutex);
		connfd = clifd[iget];
		if (++iget == MAXNCLL)//当前处理完的套接字个数达到最大值
			iget = 0;
		pthread_mutex_unlock(&clifd_mutex);//操作共享变量完毕，互斥锁解锁
		ptr[(int)arg].thread_count++;//线程已处理完毕的套接字个数+1
		web_child(connfd);//调用事务处理函数
		close(connfd);
	}
}

void sig_int(int signo)//信号处理函数
{
	int i;
	for (i = 0; i < nthreads; i++)
		printf("thread %d,%d connections\n", i, ptr[i].thread_count);
	exit(0);
}

void web_child(int sockfd)//事务处理函数
{
	int ntowrite;
	ssize_t nread;
	char line[MAXLINE], result[MAXN];
	for (;;) 
	{
		if ((nread = recv(sockfd, line, MAXLINE, 0)) = 0)//若读取的数据长度为0
			return;
		printf("receive from client [%s]\n", line);
		ntowrite = atol(line);//计算读取的字符数
		if ((ntowrite <= 0) || (ntowrite > MAXN))
		{
			printf("client request for %d bytes\n", ntowrite);
			return;
		}
		printf("send to client [%s]\n", result);
		writen(sockfd, result, ntowrite);//调用该函数发送ntowrite字节个任意的数据给客户端
	}
}

ssize_t writen(int fd, const void* vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char* ptr;
	ptr = (const char*)vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ((nwritten = send(fd, ptr, nleft, 0)) <= 0)
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return(-1);
		nleft -= nwritten;
		ptr += nwritten;
	}
	return(n);
}


