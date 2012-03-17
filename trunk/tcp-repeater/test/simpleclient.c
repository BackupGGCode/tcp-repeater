#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#define N 20
#define TCP_PORT 16665
struct sockaddr_in serv;
void *
send_request(){
    int sockfd;
    int optval = 1;
    while(1){
        sockfd = socket(AF_INET,SOCK_STREAM,0);
        if(sockfd <0){
            perror("socket error");
            pthread_exit(NULL);
        }
        int err = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
        if(err <0){
            perror("setsockopt error");
            pthread_exit(NULL);
        }
        if(connect(sockfd,(struct sockaddr*)&serv,sizeof(struct sockaddr_in))<0){
            perror("connect error");
            pthread_exit(NULL);
        }
        char buf[N] = "fighting";
        if(send(sockfd,(void*)buf,sizeof(buf),0)!=sizeof(buf)){
            perror("send error!");
            continue;
        }
        printf("Send ok!\n");
        bzero((void*)buf,N);
        if(recv(sockfd,(void*)buf,N,0)<0){
            perror("recv error");
            continue;
        }
        printf("receive :%s\n",buf);
        close(sockfd);
    }
    return NULL;
}

int 
main(int argc, char *argv[]){
    int thread_number;
    if(argc !=5){
        printf("useage:./client -t thread_number -s server_ip\n");
        return 0;
    }
    thread_number = atoi(argv[2]);
    serv.sin_addr.s_addr = inet_addr(argv[4]);
    serv.sin_port =htons(TCP_PORT);
    serv.sin_family=AF_INET;
    printf("the thread number = %d\t,port number = %d\t, server ip = %s\n",thread_number,TCP_PORT,inet_ntoa(serv.sin_addr));
    int i;
    pthread_t tid;
    for(i=0;i<thread_number;i++){
        pthread_create(&tid,NULL,&send_request,NULL);
    }
    for(;;)
        pause();
    return 0;
}
