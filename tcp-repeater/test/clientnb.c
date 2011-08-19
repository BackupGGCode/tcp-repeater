#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../include/transfer.h"
#include "../include/message.h"
#include "../include/nodetype.h"

struct sockaddr_in serv;
void *
send_request(void *arg){
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
        if(connect_nonb(sockfd,(struct sockaddr*)&serv,sizeof(struct sockaddr_in),10)<0){
            printf("connect timeout\n");
            pthread_exit(NULL);
        }
        struct message *msg = (struct message*)arg;
        printf("send TCP_SERV message to server\n");
        msg->msgtype = TCP_SERV;
        if(send(sockfd,(void*)msg,sizeof(struct message),0)!=sizeof(struct message)){
            perror("send error!");
            continue;
        }
        if(recv(sockfd,(void*)msg,sizeof(struct message),0)<0){
            perror("recv error");
            continue;
        }
        if(msg->msgtype == TCP_SERV_REPLY){
            printf("visit the server success!\n");
        }else if(msg->msgtype == TCP_SERV_FAULT){
            printf("should try again\n");
        }else{
            printf("receive error\n");
        }
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
    struct message msg;
    msg.msgtype = TCP_SERV;
    msg.info.srcNode = serv.sin_addr.s_addr;
    int i;
    pthread_t tid;
    for(i=0;i<thread_number;i++){
        pthread_create(&tid,NULL,&send_request,&msg);
    }
    for(;;)
        pause();
    return 0;
}
