#include "realserv.h"
#include "../include/nodetype.h"
#include "../include/message.h"

/*
   initialize the backend node structure
*/
struct childnode*
init_childnode(){
    struct childnode *child = (struct childnode*)malloc(sizeof(struct childnode));
    if(child ==NULL){
        err_sys("malloc error");
    }
    child->links = 0;
    child->state = INITIAL_STATE;
    //child->links =  0;
    child->tcp_threads = 2;
    Pthread_mutex_init(&(child->lock),NULL);  
    return child;
}

/*
   the function of read configure file of backend
*/

void
read_realserv_config(struct childnode* node){
    char line[MAXLINE],key[MAXLINE],value[MAXLINE];
    FILE *fp;
    fp = Fopen("config/node.ini","r");
    while(Fgets(line,MAXLINE,fp)){
        if(feof(fp))
            break;
        if(line[0] =='#')
            continue;
        else if(line[0]=='\0' || line[0]=='\n' || line[0]==' ')
            continue;
        sscanf(line,"%s = %s\n",key,value);
        if(!strcmp(key,"father_ip")){
            node->father_ip = inet_addr(value);
        }else if(!strcmp(key,"max_links")){
            node->max_links = atoi(value);
        }else if(!strcmp(key,"node_type")){
            if(strcmp(value,"LB")!=0){
                err_quit("read configure file error!\n");
            }
        }
    } 
}

/*
   the function of send heartbeat message to frontend node
*/
void*
send_heartbeat(void *arg){
    struct childnode *child = (struct childnode*)(arg);
    struct sockaddr_in serv;
    int n;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(UDP_PORT);
    serv.sin_addr.s_addr = child->father_ip;
    int sockfd = Socket(AF_INET,SOCK_DGRAM,0);
    struct message msg ;
    msg.msgtype = UDP_HEARTBEAT;
    Pthread_mutex_lock(&(child->lock));
    msg.child_id = child->id;
    msg.state = child->state;
    msg.info.links = child->links;
    Pthread_mutex_unlock(&(child->lock));
    while(1){
        if((n = sendto(sockfd,(void*)&msg,sizeof(msg),0,(struct sockaddr*)&serv,
                        sizeof(serv))) != sizeof(struct message)){
            perror("send heartbeat error");
        }
        sleep(HEARTBEAT_RATE);
    }
    return NULL;
}

/*
    the function of send TCP_ACTIVATION message to frontend machine
*/
void* 
send_activation_message(void *arg){
    int sockfd;
    struct sockaddr_in serv;
    struct childnode *child = (struct childnode*)arg;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(TCP_PORT);
    serv.sin_addr.s_addr = child->father_ip;
    sockfd = Socket(AF_INET,SOCK_STREAM,0);
    
    struct message msg;
    msg.msgtype  = TCP_ACTIVATION;
    msg.child_id = 0;
    Pthread_mutex_lock(&(child->lock));

    msg.state = child->state;
    Pthread_mutex_unlock(&(child->lock));
    Connect(sockfd,(struct sockaddr*)&serv,sizeof(serv));
    Send(sockfd,&msg,sizeof(msg),0);
    printf("register to front server......\n");
    int n= Recv(sockfd,&msg,sizeof(msg),0);
    if(n != sizeof(msg))
        err_quit("receive activation ack message error");
    if(msg.msgtype ==TCP_ACTIVATION_ACK){
        Pthread_mutex_lock(&(child->lock));
        child->id = msg.child_id;
        child->state = NORMAL_STATE;
        Pthread_mutex_unlock(&(child->lock));
    }
    //printf("Get activation ack message from traffic server !\n");
    printf("the server is in normal state!\n");
    close(sockfd);
    return NULL;
}


/*****************process tcp message ***********************/

int tcp_childfd;            //the tcp server socket
pthread_mutex_t tcp_childsock_lock = PTHREAD_MUTEX_INITIALIZER;     //the lock of tcp server socket


/*
   tcp message handler in backend
*/
void 
recv_tcp_message(struct childnode *child,int connfd,in_addr_t srcaddr){
    char buf[PACKET_LEN];
    int len;
    struct message *msg;
    struct in_addr addr;
    addr.s_addr = srcaddr;
//    printf("receive message from %s\n",inet_ntoa(addr));
    for(;;){
        len = Recv(connfd,(void*)buf,PACKET_LEN,0);
        if(len ==0){
            printf("the connection was closed by %s!\n",inet_ntoa(addr));
            break;
        }else if(len <0){
            break;
        }
        msg = (struct message*)buf;
        switch(msg->msgtype){
            case TCP_FAULT_QUERY:
                printf("receive TCP_FAULT_QUERY message\n");
                msg->msgtype = TCP_FAULT_QUERY_ACK;
                Send(connfd,msg,sizeof(struct message),0);
                break;
            case TCP_SERV:
                printf("receive TCP_SERV message\n");
                msg->msgtype =TCP_SERV_REPLY;
                while(send(connfd,msg,sizeof(struct message),0)!=sizeof(struct message)){
                        continue;
                }
                printf("send TCP_SERV_REPLY message\n");
                break;
            default:
                break;
        }
    }
    //close(connfd);
}


/*
   the process program of every single thread,used for tcp
*/
void *
child_tcp_thread(void *arg){
    struct childnode *child = (struct childnode*)arg;
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    for(;;){
        clilen = sizeof(struct sockaddr_in);
        Pthread_mutex_lock(&tcp_childsock_lock);
        connfd = Accept(tcp_childfd,(struct sockaddr*)&cliaddr,&clilen);
        Pthread_mutex_unlock(&tcp_childsock_lock);
//        printf("the message was from %s\n",inet_ntoa(cliaddr.sin_addr));
        recv_tcp_message(child,connfd,cliaddr.sin_addr.s_addr);
        close(connfd);
    }
    return NULL;
}

/*
   the tcp service program entry, this function will create a server socket
   and create many threads to process tcp message
*/
void *
process_tcp_message(void *arg){
    struct childnode *child = (struct childnode*)arg;
    struct sockaddr_in serv;
    int i,optval =1;
    pthread_t tid;
    bzero(&serv,sizeof(struct sockaddr_in));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(TCP_PORT);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_childfd = socket(AF_INET,SOCK_STREAM,0);
    if(tcp_childfd<0){
        perror("socket error");
        exit(1);
    }
    int err = setsockopt(tcp_childfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(err){
        perror("setsockopt error");
        exit(1);
    }
    if(bind(tcp_childfd,(struct sockaddr*)&serv,sizeof(struct sockaddr_in))<0){
        perror("bind error");
        exit(1);
    }
    if(listen(tcp_childfd,LISTENQ)<0){
        perror("listen error");
        exit(1);
    }
    for(i=0;i<child->tcp_threads;i++){
        Pthread_create(&tid,NULL,&child_tcp_thread,(void*)child);
    }
    return NULL;    
}

/*
   the key function that start the service of backend machine,
   includeing heartbeat service,tcp service
*/
void 
realserv_start(){
    pthread_t tid;
    struct childnode *child = init_childnode();
    read_realserv_config(child);
    send_activation_message(child);    
    if(child->state==NORMAL_STATE){
        Pthread_create(&tid,NULL,&send_heartbeat,(void*)child);
        Pthread_create(&tid,NULL,&process_tcp_message,(void *)child);
    }
    for(;;)
        pause();
}



