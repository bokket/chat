#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/un.h>

void sys_err(const char* s,int line);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);
int Close(int fd);
ssize_t Readn(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);
static ssize_t my_read(int fd, char *ptr);
ssize_t Readline(int fd, void *vptr, size_t maxlen);


void sys_err(const char* s,int line)
{
    fprintf(stderr,"line:%d",line);
    perror(s);
    exit(1);
}
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n=0;

again:
	if ((n=accept(fd,sa,salenptr))<0) 
    {
		if ((errno==ECONNABORTED)||(errno==EINTR))
			goto again;
		else
			sys_err("accept error",__LINE__);
	}
	return n;
}
int Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
    int n=0;
	if((n=bind(fd,sa,salen))<0)
		sys_err("bind error",__LINE__);
    return n;
}
int Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{

    int n=0;

	if((n=connect(fd,sa,salen))<0)
		sys_err("connect error",__LINE__);
    return n;
}
int Listen(int fd, int backlog)
{
    int n;
	if((n=listen(fd,backlog))<0)
		sys_err("listen error",__LINE__);
    return n;
}
int Socket(int family, int type, int protocol)
{
   int n;
	if((n=socket(family,type,protocol))<0)
		sys_err("socket error",__LINE__);
	return n;
}
ssize_t Read(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;
again:
	if ((n=read(fd,ptr,nbytes))==-1) 
    {
		if(errno==EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}
ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
    ssize_t n;

again:
	if((n=write(fd,ptr,nbytes))==-1)
    {
		if(errno==EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}
int Close(int fd)
{
    int n;
	if((n=close(fd))==-1)
		sys_err("close error",__LINE__);

    return n;
}


/*参三: 应该读取的字节数*/
ssize_t Readn(int fd, void *vptr, size_t n)
{
	size_t  nleft;              //usigned int 剩余未读取的字节数
	ssize_t nread;              //int 实际读到的字节数
	char   *ptr;

	ptr = vptr;
	nleft = n;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;
			else
				return -1;
		} else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return n - nleft;
}

ssize_t Writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

static ssize_t my_read(int fd, char *ptr)
{
	static int read_cnt;
	static char *read_ptr;
	static char read_buf[100];

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return -1;
		} else if (read_cnt == 0)
			return 0;
		read_ptr = read_buf;
	}
	read_cnt--;
	*ptr = *read_ptr++;

	return 1;
}

ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char    c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c  == '\n')
				break;
		} else if (rc == 0) {
			*ptr = 0;
			return n - 1;
		} else
			return -1;
	}
	*ptr  = 0;

	return n;
}

