#ifndef _TRANSFER_H
#define _TRANSFER_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<pthread.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<linux/types.h>

//#include<asm/atomic.h>
#define HEARTBEAT_RATE  5	
#define UDP_PORT		16666
#define TCP_PORT		16665
#define SERV_PORT		16664
#define MAXLINE			4096
#define LISTENQ         100
#define PACKET_LEN      1700
#define LB_NUM          100



int Socket(int ,int ,int);
void Bind(int ,const struct sockaddr *,socklen_t);
int Accept(int ,struct sockaddr *,socklen_t *);
void Connect(int,const struct sockaddr *,socklen_t );
void Listen(int ,int);

ssize_t	 Readline(int, void *, size_t);
ssize_t	 Readn(int, void *, size_t);
ssize_t	 Recv(int, void *, size_t, int);
ssize_t	 Recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t	 Recvmsg(int, struct msghdr *, int);

int		 Select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

void	 Send(int, const void *, size_t, int);
void	 Sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
void	 Sendmsg(int, const struct msghdr *, int);
void	 Setsockopt(int, int, int, const void *, socklen_t);

void	 Shutdown(int, int);
int		 Sockatmark(int);
int		 Socket(int, int, int);
void	 Socketpair(int, int, int, int *);
void	 Writen(int, void *, size_t);


void	 err_dump(const char *, ...);
void	 err_msg(const char *, ...);
void	 err_quit(const char *, ...);
void	 err_ret(const char *, ...);
void	 err_sys(const char *, ...);

void    Fclose(FILE *);
FILE	*Fdopen(int, const char *);
char	*Fgets(char *, int, FILE *);
FILE	*Fopen(const char *, const char *);
void	 Fputs(const char *, FILE *);

//pthread function 
void	Pthread_create(pthread_t *, const pthread_attr_t *,
					   void * (*)(void *), void *);
void	Pthread_join(pthread_t, void **);
void	Pthread_detach(pthread_t);
void	Pthread_kill(pthread_t, int);

void	Pthread_mutexattr_init(pthread_mutexattr_t *);
void	Pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
void	Pthread_mutex_init(pthread_mutex_t *, pthread_mutexattr_t *);
void	Pthread_mutex_lock(pthread_mutex_t *);
void	Pthread_mutex_unlock(pthread_mutex_t *);

void	Pthread_cond_broadcast(pthread_cond_t *);
void	Pthread_cond_signal(pthread_cond_t *);
void	Pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
void	Pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *,
							   const struct timespec *);

void	Pthread_key_create(pthread_key_t *, void (*)(void *));
void	Pthread_setspecific(pthread_key_t, const void *);
void	Pthread_once(pthread_once_t *, void (*)(void));


#endif
