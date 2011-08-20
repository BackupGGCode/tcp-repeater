#include "lb_list.h"
#include "../include/nodetype.h"
#include "../include/message.h"

/*
   initialize the structure of backend machines
*/
struct lb_struct*
init_lb_hlist(in_addr_t node[],__u32 num){
    struct lb_struct *lb_hlist = (struct lb_struct*)malloc(sizeof(struct lb_struct)*num);
    if(lb_hlist ==NULL){
        printf("malloc lb_struct struct error!\n");
        return NULL;
    }
    int i;
    for(i = 0;i<num;i++){
        lb_hlist[i].state = INITIAL_STATE;
        lb_hlist[i].count = 0;
        lb_hlist[i].ipaddr = node[i];
        //lb_hlist[i].connfd = -1;
        Pthread_mutex_init(&(lb_hlist[i].lb_lock),NULL);
    }
    return lb_hlist;
}

/*
   check if the backend in normal state
*/
int is_normal(struct lb_struct *lb_hlist,int id){
    int value;
    Pthread_mutex_lock(&(lb_hlist[id].lb_lock));
    value = lb_hlist[id].state;
    Pthread_mutex_unlock(&(lb_hlist[id].lb_lock));
    if(value ==NORMAL_STATE){
        return 1;
    }else{
        return 0;
    }
}

/*
   the round robin scheduling method
*/
int
round_robin_scheduling(struct lb_struct *lb_hlist,__u32 cur,struct message *msg,int connfd){
    int sockfd;
    int len,optval = 1;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd <0){
        perror("socket error");
        return -1;
    }
    Setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(optval));
	Setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,(const void *)&optval,sizeof(optval));
	int value = 1;
	Setsockopt(sockfd,SOL_TCP,TCP_KEEPCNT,(const void*)&value,sizeof(value));
	Setsockopt(sockfd,SOL_TCP,TCP_KEEPIDLE,(const void*)&value,sizeof(value));
	optval = 2;
	Setsockopt(sockfd,SOL_TCP,TCP_KEEPINTVL,(const void*)&optval,sizeof(optval));

    bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(TCP_PORT);
	servaddr.sin_addr.s_addr = lb_hlist[cur].ipaddr;
    struct in_addr addr;
    addr.s_addr = lb_hlist[cur].ipaddr;
    printf("choose the machine:%s\n",inet_ntoa(addr));
    if(connect_nonb(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr),TIME_OUT)<0){
        close(sockfd);
        return -2;

    }
    while(1){
        /*while(send(sockfd,msg,sizeof(struct message),0)!=sizeof(struct message)){
            continue;
        }
        */

        len = send(sockfd,msg,sizeof(struct message),0);
        if(len !=sizeof(struct message)){
            continue;
        }
        while((len = recv(sockfd,(void*)msg,sizeof(struct message),MSG_DONTWAIT))!=sizeof(struct message)){
            if(errno ==ECONNRESET){
                perror("recv error");
                return -2;
            }else
                continue;
        }

        //printf("len = %d,sizeof struct message = %d\n",len,sizeof(struct message));
        if(msg->msgtype !=TCP_SERV_REPLY){
            printf("receive TCP_SERV_REPLY  message error,it is %d\n",msg->msgtype);
            continue;
        }
        while(send(connfd,msg,sizeof(struct message),0)!=sizeof(struct message)){
            continue;  
        }
        printf("send TCP_SERV_REPLY message success!\n");
        break;
    }
    /*printf("round_robin_send to backend\n"); 
    if(send(sockfd,msg,sizeof(struct message),MSG_DONTWAIT)!=sizeof(struct message)){
        msg->msgtype = TCP_SERV_FAULT;
        if(send(connfd,msg,sizeof(struct message),MSG_DONTWAIT)!=sizeof(struct message)){
            return -3;
        } 
    }
    printf("round_robin_recv from backend\n");
    if((len = recv(sockfd,(void*)msg,sizeof(struct message),MSG_DONTWAIT))!=sizeof(struct message)){
        msg->msgtype = TCP_SERV_FAULT;
        if(send(connfd,msg,sizeof(struct message),MSG_DONTWAIT)!=sizeof(struct message)){
            return -3;
        }
    }
        //printf("len = %d,sizeof struct message = %d\n",len,sizeof(struct message));
    if(msg->msgtype !=TCP_SERV_REPLY){
        printf("receive TCP_SERV_REPLY  message error,it is %d\n",msg->msgtype);
        msg->msgtype = TCP_SERV_FAULT;
        if(send(connfd,msg,sizeof(struct message),MSG_DONTWAIT)!=sizeof(struct message)){
            return -3;
        }
     }
    printf("round robin send to client!\n");
    if(send(connfd,msg,sizeof(struct message),MSG_DONTWAIT)!=sizeof(struct message)){
        return -3;
    }*/
    //printf("send TCP_SERV_REPLY message success!\n");
    close(sockfd); 
    return 0;
    /*
    Pthread_mutex_lock(&(lb_hlist[cur].lb_lock));
    if(lb_hlist[cur].connfd ==-1){
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(TCP_PORT);
	    servaddr.sin_addr.s_addr = lb_hlist[cur].ipaddr;
	    lb_hlist[cur].connfd = Socket(AF_INET,SOCK_STREAM,0);
	    Send(lb_hlist[cur].connfd,msg,sizeof(struct message),0);
	 
    }else{
        if(send(lb_hlist[cur].connfd,msg,sizeof(struct message),0) != sizeof(struct message)){
	        close(lb_hlist[cur].connfd);
	        servaddr.sin_family = AF_INET;
	        servaddr.sin_port = htons(TCP_PORT);
	        servaddr.sin_addr.s_addr = lb_hlist[cur].ipaddr;
	        lb_hlist[cur].connfd = Socket(AF_INET,SOCK_STREAM,0);
	    }
    }
    Recv(lb_hlist[cur].connfd,(void*)msg,sizeof(struct message),0);
    Pthread_mutex_unlock(&(lb_hlist[cur].lb_lock));
    if(msg->msgtype==TCP_SERV_REPLY){
	    return sizeof(struct message);
    }else{
	    printf("receive wrong message!\n");
	    return 0;
    }*/

}

/*
   the src address hash function
*/
int
srcaddr_hash_scheduling(struct lb_struct * lb_hlist,in_addr_t srcaddr,__u32 hash_size,struct message *msg,int connfd){
    __u32 cur;
    cur = srcaddr %hash_size;
    int sockfd;
    int len,optval =1;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        perror("socket error");
        return -1;
    }
    int err = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(err){
        perror("setsockopt error");
        return -1;
    } 
    while(is_normal(lb_hlist,cur)){
        bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(TCP_PORT);
	    servaddr.sin_addr.s_addr = lb_hlist[cur].ipaddr;
        if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
            //perror("connect error");
            //close(sockfd);
            cur = (cur + 1)%hash_size;
            continue;
        }
        while(send(sockfd,msg,sizeof(struct message),0)!=sizeof(struct message)){
            continue;
        }
        if(recv(sockfd,(void*)msg,sizeof(struct message),0)!=sizeof(struct message)){
            continue;
        }
        if(msg->msgtype !=TCP_SERV_REPLY){
            printf("receive message error!\n");
            msg->msgtype = TCP_SERV;
            continue;
        }
        while((len=send(connfd,msg,sizeof(struct message),0))!=sizeof(struct message)){
            if(len ==-1){
                perror("send error");
                return 0;
            }
            continue;  
        }
        break;
    }
    close(sockfd);
    return 0;
}

/*
   the process function when receiving the heartbeat message from a fault backend machine
*/
void *
fault_query(void * arg){
    int optval = 1;
    struct lb_struct *lb = (struct lb_struct *)arg;
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    servaddr.sin_addr.s_addr = lb->ipaddr;
    int sockfd = Socket(AF_INET,SOCK_STREAM,0);
    int err = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(err){
        perror("setsockopt error");
        pthread_exit(NULL);
    }
    struct in_addr addr;
    addr.s_addr = lb->ipaddr;
    if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
       perror("connect error");
       pthread_exit(NULL);
    }
    struct message msg;
    msg.msgtype =TCP_FAULT_QUERY;
    if(send(sockfd,&msg,sizeof(struct message),0)!=sizeof(struct message)){
        perror("send error");
        pthread_exit(NULL);
    }
    printf("get heartbeat message from %s again,send TCP_FAULT_QUERY message!\n",inet_ntoa(addr));
    if(recv(sockfd,(void*)&msg,sizeof(struct message),0)!=sizeof(struct message)){
        perror("send error");
        pthread_exit(NULL);
        
    }
    if( msg.msgtype != TCP_FAULT_QUERY_ACK){
        pthread_exit(NULL);
    }
        
    Pthread_mutex_lock(&(lb->lb_lock));
    lb->state =NORMAL_STATE;
    lb->count = 0;
    Pthread_mutex_unlock(&(lb->lb_lock));
    printf("the machine %s is in normal state!\n",inet_ntoa(addr));
    return NULL;
}

/*
   the heartbeat message handler in lb_struct
*/
void 
get_heartbeat(struct lb_struct *lb_hlist,__u32 id,__u32 hash_size){
    if(id >= hash_size){
        err_msg("wrong heartbeat message!\n");
    }
    struct in_addr addr;
    addr.s_addr = lb_hlist[id].ipaddr;
    //printf("get heartbeat message from %s\n",inet_ntoa(addr));
    Pthread_mutex_lock(&(lb_hlist[id].lb_lock));
    if(lb_hlist[id].state ==NORMAL_STATE){
        lb_hlist[id].count = 0;
        Pthread_mutex_unlock(&(lb_hlist[id].lb_lock));
    }else if(lb_hlist[id].state == FAULT_STATE){//recover from the fault state ?
        Pthread_mutex_unlock(&(lb_hlist[id].lb_lock));
        pthread_t tid;
        Pthread_create(&tid,NULL,&fault_query,(void*)(lb_hlist+id));
    }else{
        Pthread_mutex_unlock(&(lb_hlist[id].lb_lock));
    }
}

/*
   check the hearbeat count of each backend machine
*/
void 
heartbeat_count(struct lb_struct *lb_hlist,__u32 hash_size,__u32 max_count){
    __u32 i;
    struct in_addr addr;
    for(i=0;i<hash_size;i++){
        Pthread_mutex_lock(&(lb_hlist[i].lb_lock));
        if(lb_hlist[i].state==NORMAL_STATE)
            lb_hlist[i].count  = lb_hlist[i].count +1; ;
        /*
        */
        addr.s_addr = lb_hlist[i].ipaddr;
        if(lb_hlist[i].count > max_count){
            printf("the count of %s:%d\n",inet_ntoa(addr),lb_hlist[i].count);
            lb_hlist[i].state = FAULT_STATE;
            struct in_addr addr;
            addr.s_addr = lb_hlist[i].ipaddr;
            printf("%s is in fault state!\n",inet_ntoa(addr));
        }
        Pthread_mutex_unlock(&(lb_hlist[i].lb_lock));
    }
}

/*
   print the ip address the backend machine
*/
void 
print_lb_hlist(struct lb_struct *lb_hlist,int hash_size){
    struct in_addr ip;
    int i;
    for( i=0;i<hash_size;i++){
        printf(" |\n");
        printf(" |\n");
        printf("  --------");
        ip.s_addr = lb_hlist[i].ipaddr;
        printf("%s\n",inet_ntoa(ip));
    }
}

/*
   the tcp message handler in lb_struct 
*/
__u32 
process_message_lb(struct lb_struct *lb_hlist,__u32 hash_size,in_addr_t srcaddr,struct message *msg){
    int i, len;
    len =0;
    switch(msg->msgtype){
        case TCP_ACTIVATION:
            if(msg->state == INITIAL_STATE){
                for(i=0;i<hash_size;i++){
                    if(lb_hlist[i].ipaddr == srcaddr){
                        msg->child_id = i;
                        break;
                    }
                }
                Pthread_mutex_lock(&(lb_hlist[i].lb_lock));
                lb_hlist[i].state = NORMAL_STATE;
                lb_hlist[i].count = 0;
                Pthread_mutex_unlock(&(lb_hlist[i].lb_lock));
            }
            msg->msgtype = TCP_ACTIVATION_ACK;
            len = sizeof(struct message);
	        break;
	    default:
            break;
    }
    return len;
}
