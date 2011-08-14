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
    child->client_size = 1024;
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
    if(0!=connect(sockfd,(struct sockaddr*)&serv,sizeof(serv))){
		perror("connect error");
		exit(1);
	}
    while(1){
        int len =send(sockfd,&msg,sizeof(msg),0);
        if(len != sizeof(msg)){
            perror("send error");
            continue;
        }
        printf("register to front server......\n");
        int n= recv(sockfd,&msg,sizeof(msg),0);
        if(n != sizeof(msg)){
            perror("recv error");
            continue;
        }
        if(msg.msgtype ==TCP_ACTIVATION_ACK){
            Pthread_mutex_lock(&(child->lock));
            child->id = msg.child_id;
            child->state = NORMAL_STATE;
            Pthread_mutex_unlock(&(child->lock));
            break;
        }
    }
    //printf("Get activation ack message from traffic server !\n");
    printf("the server is in normal state!\n");
    close(sockfd);
    return NULL;
}


/*****************process tcp message ***********************/


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
   the tcp service program entry, this function will create a server socket
   and create many threads to process tcp message
*/
void *
process_tcp_message(void *arg){
    struct childnode *child = (struct childnode*)arg;
    struct sockaddr_in serv;
    int optval =1;
    bzero(&serv,sizeof(struct sockaddr_in));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(TCP_PORT);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    int tcp_childfd = socket(AF_INET,SOCK_STREAM,0);
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
    select_server(child,tcp_childfd);
    return NULL;    
}

void select_server(struct childnode *child,int tcp_listenfd){
   int maxfd,maxIndex,i;
   struct timeval timeout;
   int *client = (int*)malloc(sizeof(int) * child->client_size);
   struct sockaddr_in cliaddr;
   int clilen;
   fd_set allset,rset;
   FD_ZERO(&allset);
   FD_ZERO(&rset);
   FD_SET(tcp_listenfd,&allset);
   maxfd = tcp_listenfd +1;
   maxIndex = 0;
   for( i=0;i<child->client_size;i++)
       client[i]=-1;
   timeout.tv_sec = 20;
   timeout.tv_usec=0;
   int nready;
   printf("the backend server ready!\n");
   for(;;){
       rset = allset;
       nready = select(maxfd+1,&rset,NULL,NULL,&timeout);
	   if(nready ==0)
		   continue;
	   printf("nready = %d\n",nready);	
	   if(FD_ISSET(tcp_listenfd,&rset)){
           clilen = sizeof(cliaddr);
           int connfd = Accept(tcp_listenfd,(struct sockaddr*) & cliaddr,(socklen_t*)&clilen);
		   if(connfd ==-1){
				perror("accept error");
				continue;
		   }
		   printf("get a request,Accept it!\n");
		   for(i=0;i<child->client_size;i++){
                if(client[i]==-1)
                    break;
           }
		   if(i>=child->client_size){
			   printf("too few client connfd buffer!\n");
			   exit(1);
		   }
           client[i] = connfd;
           if( connfd > maxfd)
               maxfd = connfd;
           if( i> maxIndex)
               maxIndex = i;
           FD_SET(connfd,&allset);
           if(--nready <=0)
               continue;
       }
       for(i=0;i<=maxIndex;i++) {
		   printf("receive packet...\n");
           if(client[i]==-1){
               continue;
           }
           if(FD_ISSET(client[i],&rset)){
                int flag = readPacket(child,client[i]);
                printf("read packet over!\n");
                if(flag ==0){
                    close(client[i]);
                    FD_CLR(client[i],&allset);
                    client[i]=-1;
                }
                nready--;
                if(nready<=0)
                    break;
           }
       }
   }
}


int readPacket(struct childnode *child,int connfd){
	printf("read packet!\n");
    char buf[PACKET_LEN];
    int len;
    struct message *msg;
    struct in_addr addr;
//    printf("receive message from %s\n",inet_ntoa(addr));
    for(;;){
		printf("just after for...\n");
        len = Recv(connfd,(void*)buf,PACKET_LEN,0);
        msg = (struct message *)buf;
		addr.s_addr = msg->info.srcNode;
        printf("if...\n");
		if(len ==0){
            printf("the connection was closed by %s!\n",inet_ntoa(addr));
            return 0;
        }else if(len <0){
            break;
        }
        msg = (struct message*)buf;
		printf("swich ...\n");
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
    return 0;
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
    Pthread_create(&tid,NULL,&process_tcp_message,(void *)child);
    send_activation_message(child);    
    if(child->state==NORMAL_STATE){
        Pthread_create(&tid,NULL,&send_heartbeat,(void*)child);
    }
    for(;;)
        pause();
}



