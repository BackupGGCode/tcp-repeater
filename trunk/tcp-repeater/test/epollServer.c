#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<netinet/in.h>


#define BUFF_SIZE 1024
#define SERV_PORT 16665
#define CLIENT_SIZE 1024
#define LISTENQ 10

int addNewClient(int epfd,int listenfd){
    struct epoll_event ev;
    struct sockaddr_in cliaddr;
    int len  = sizeof(cliaddr);
    int connfd =  accept(listenfd,(struct sockaddr *)&cliaddr,&len);
    if(connfd <0){
        perror("accept error!");
        return -1;
    }
    ev.data.fd = connfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
    return connfd;
}

int readPacket(int connfd){
    char buf[BUFF_SIZE];
    int len = recv(connfd,(void*)buf,BUFF_SIZE,0);
    if(len <0){
        perror("recv error");
        return -1;
    }
    else if( len ==0){
        return 0;
    }else{
        printf("%s\n",buf);
        send(connfd,(void*)buf,len,0);
        return 1;
    }
}

int epollServer(int listenfd){
    int epfd,i;
    struct epoll_event listen_ev,events[CLIENT_SIZE];
    epfd = epoll_create(10);
    listen_ev.events = EPOLLIN;
    listen_ev.data.fd = listenfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&listen_ev);
    int nready;
    for(;;){
        nready = epoll_wait(epfd,events,CLIENT_SIZE,-1);
        if(nready <0){
            perror("epoll_wait error");
            return -1;
        }
        for(i =0;i<nready;i++){
            if(events[i].data.fd ==listenfd){
                addNewClient(epfd,listenfd);
            }else if(events[i].events & EPOLLIN){
                int flag = readPacket(events[i].data.fd);
                if(flag ==0){
                    close(events[i].data.fd);
                    epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,events+i);
                    
                }
            }else if(events[i].events & EPOLLERR){
                close(events[i].data.fd);
                printf("connection error\n");
                epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,events+i);
            }
            
        }
    }
    return 0;
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


    epollServer(listenfd);
    
    return 0;
}

