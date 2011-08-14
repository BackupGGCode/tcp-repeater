#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<strings.h>
#include<signal.h>
#include<pthread.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/times.h>
#include<sys/select.h>
#include<netinet/in.h>

#define CLIENT_SIZE 1024
#define SERV_PORT 16665
#define LISTENQ 1000
#define BUFF_SIZE 1024

int client[CLIENT_SIZE];
unsigned long long  links =0;
pthread_mutex_t mutex;


void handler(){
    pthread_mutex_lock(&mutex);
    printf("links = %lld\n",links);
    links = 0;
    alarm(2);
    pthread_mutex_unlock(&mutex);
}

int addNewClient(int listenfd,int * maxfd,int * maxIndex){
    struct sockaddr_in client_addr;
    socklen_t len;
    int i;
    len = sizeof(client_addr);
    int connfd = accept(listenfd,(struct sockaddr*)&client_addr,&len);
    if(connfd <0){
        perror("accept error");
        return -1;
    }
    pthread_mutex_lock(&mutex);
    if(links ==0){
        alarm(2);
    }
    links ++;
    pthread_mutex_unlock(&mutex);
    if( connfd > *maxfd)
        *maxfd = connfd;
    for(i=0;i<=*maxIndex;i++){
        if(client[i]<0){
            client[i] = connfd;
            break;
        }
    }
    if( i> *maxIndex){
        *maxIndex = *maxIndex +1;
        client[*maxIndex] = connfd;
    }
    return connfd;
}

int readPacket(int connfd){
    char buf[BUFF_SIZE];
    int len;
    len = recv(connfd,(void*)buf,BUFF_SIZE,0);
    if(len <0){
        perror("receive error");
        return -1;
    }
    if(len ==0)
        return 0;
    else{
        printf("%s\n",buf);
        send(connfd,(void*)buf,len,0);
        return 1;
    }
}

int selectServer(int listenfd){
    int maxfd,maxIndex;
    int i;
    fd_set rset,allset;
    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);
    maxfd  = listenfd +1;
    maxIndex =0;
    int nready;
    for(i=0;i<CLIENT_SIZE;i++)
        client[i]=-1;
    for(;;){
        rset = allset;
        printf("start select...\n");
        nready = select(maxfd,&rset,NULL,NULL,NULL);
        printf("nready = %d\n",nready);
        if( FD_ISSET(listenfd,&rset)){
            int connfd = addNewClient(listenfd,&maxfd,&maxIndex);
            FD_SET(connfd,&allset);
            nready --;
            if( nready <=0)
                continue;
        }
        for(i=0;i<=maxIndex;i++){
            if(client[i]<0)
                continue;
            if(FD_ISSET(client[i],&rset)){
                int flag = readPacket(client[i]);
                if(flag ==0){
                    close(client[i]);
                    FD_CLR(client[i],&allset);
                    client[i]=-1;
                }
                nready--;
                if(nready <=0)
                    break;
            }
        }
    }
}

int main(int argc,char *argv[]){
    int listenfd;
    struct sockaddr_in serv_addr;
    listenfd =  socket(AF_INET,SOCK_STREAM,0);
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);
    bind(listenfd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr));
    listen(listenfd,LISTENQ);
    pthread_mutex_init(&mutex,NULL);
    signal(SIGALRM,handler);
    selectServer(listenfd);
    return 0;

}
