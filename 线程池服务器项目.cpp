// �̳߳ط�������Ŀ.cpp : �������̨Ӧ�ó������ڵ㡣
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
#define PORT 10000 //����˿ں� 10000
#define LISTENQ 1024 //����ȴ����г���1024
#define MAXLINE 4096 //�������ݻ�������С4M
#define MAXN 1024 //����������˷��͵�����ֽ���1024
typedef struct { //�����̳߳ؽṹ��
	pthread_t thread_tid; //�߳�ID
	long thread_count; //�̴߳���Ŀͻ���������
}Thread;
Thread* ptr; //�����̳߳ؽṹ�����
#define  MAXNCLL 32 //����ɴ򿪵Ĵ��׽���������Ϊ32
int clifd[MAXNCLL], iget, iput; //clifd�����������߳������д����ѽ��ܵ��������׽����������������̳߳��еĿ����̴߳���ȡ��һ������
//��Ӧ�ͻ���iput����������������һ��Ԫ���±꣬iget�Ǵ�������ȡ����һ��Ԫ�ص��±�
pthread_mutex_t clifd_mutex = PTHREAD_MUTEX_INITIALIZER;//��ʼ������������
pthread_cond_t clifd_cond = PTHREAD_COND_INITIALIZER;	//��ʼ����������
int msock, nthreads;	//�������߳��������������̸߳�������nthreads
socklen_t addrlen;	//�����˵��ַ�ṹ���ȱ���
struct sockaddr_in* clientaddr;	//�����ͻ����׽��ֶ˵��ַ�ṹ�����
static pid_t* pids;//�����߳�����������
void web_child(int);	//������������
void sig_int(int);	//�����źŴ�����
static pthread_mutex_t* mptr;	//��������������
ssize_t read_fd(int, void*, size_t, int*);	//����read_fd����
ssize_t write_fd(int, void*, size_t, int);	//����write_fd����
void* thread_main(void*)	//�����߳���ִ�к���
void thread_make(int i);	//�����̳߳��д��̴߳�������

int main(int argc,char**argv)
{
	int i, navail, maxfd, nsel, rc;
	int ssock;
	pthread_t tid;	//�����߳�����������
	socklen_t len;	//�˵��ַ�ṹ���ȱ���
	struct sockaddr_in servaddr;	//�������׽��ֶ˵��ַ�ṹ����
	const int on = 1;
	ssize_t n;
	fd_set rset,masterset;
    
	msock = socket(AF_INET, SOCK_STREAM, 0);//�������׽��� 
	meset(&servaddr, 0, sizeof(servaddr));
	//�������׽��ֵ�ַ������ֵ��
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);
	//�������׽�������
	setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bind(msock, (struct sockaddr*)&srvaddr, sizeof(servaddr));
	listen(msock, LISTENQ);
	
	nthreads = 10;//��ʼ���̳߳��е��̸߳���Ϊ10
	ptr = (Thread*)calloc(nthreads, sizeof(Thread));//Ϊ�̳߳ؽṹ����������ڴ�
	iget = iput = 0;
	for (i = 0; i < nthreads; i++)
		thread_make(i);//Ԥ������10�����߳�
	signal(SIGINT, sig_int);//�źŴ����� sig_intΪִ�к��� �����ź�SIGINT (CTRL+O)  
	for (;;) 
	{
		len = sizeof(clientaddr);
		ssock = accept(msock, clientaddr, &len);
		//�����������clifd,iput�ȣ��Ȼ���������
		pthread_mutex_lock(&clifd_mutex); 
		clifd[iput] = ssock;

		if (++iput == MAXNCLL) {
			iput = 0;
		}
		if (++iput == iget) {
			printf("iput=iget=%d\n", iput);
			exit(-1);
		}
		pthread_cond_signal(&clifd_cond);//�ú������������������ź� �������������ϵ��߳�
		pthread_mutex_unlock(&clifd_mutex);//�������������� ����������
	}
}

void thread_make(int i) {//Ϊ�̳߳ش����µ��߳�
	void*thread_main(void*);
	pthread_create(&ptr[i].thread_tid, NULL, &thread_main, (void)i);
	return;
}

void*thread_main(void* arg)
{//�߳�ִ���庯��
	int connfd; //�׽����������ֲ�����
	void web_child(int);
	printf("thread %d starting \n", (int)arg);
	for (;;) 
	{
		//�����������iput,iget,clifd֮ǰ���ȶԻ���������
		pthread_mutex_lock(&clifd_mutex);
		printf("get lock,thread=[%d]\n", (int)arg);
		while (iget == iput)//���л���׽��־��Ѵ������
			pthread_cond_wait(&clifd_cond, &clifd_mutex);
		connfd = clifd[iget];
		if (++iget == MAXNCLL)//��ǰ��������׽��ָ����ﵽ���ֵ
			iget = 0;
		pthread_mutex_unlock(&clifd_mutex);//�������������ϣ�����������
		ptr[(int)arg].thread_count++;//�߳��Ѵ�����ϵ��׽��ָ���+1
		web_child(connfd);//������������
		close(connfd);
	}
}

void sig_int(int signo)//�źŴ�����
{
	int i;
	for (i = 0; i < nthreads; i++)
		printf("thread %d,%d connections\n", i, ptr[i].thread_count);
	exit(0);
}

void web_child(int sockfd)//��������
{
	int ntowrite;
	ssize_t nread;
	char line[MAXLINE], result[MAXN];
	for (;;) 
	{
		if ((nread = recv(sockfd, line, MAXLINE, 0)) = 0)//����ȡ�����ݳ���Ϊ0
			return;
		printf("receive from client [%s]\n", line);
		ntowrite = atol(line);//�����ȡ���ַ���
		if ((ntowrite <= 0) || (ntowrite > MAXN))
		{
			printf("client request for %d bytes\n", ntowrite);
			return;
		}
		printf("send to client [%s]\n", result);
		writen(sockfd, result, ntowrite);//���øú�������ntowrite�ֽڸ���������ݸ��ͻ���
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


