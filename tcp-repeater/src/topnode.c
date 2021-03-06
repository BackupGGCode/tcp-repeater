#include "topnode.h"
#include "../include/nodetype.h"
#include "../include/message.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern struct lb_struct *init_lb_hlist(in_addr_t node[],__u32 num);
extern struct lb_struct *init_lb_struct(in_addr_t ipaddr);
extern int round_robin_scheduling(struct lb_struct *lb_hlist,__u32 cur,struct message *msg,int connfd);
extern int srcaddr_hash_scheduling(struct lb_struct *lb_hlist,in_addr_t srcaddr,__u32 hash_size,struct message *msg,
        int connfd);
extern void get_heartbeat(struct lb_struct *lb_hlist,__u32 id,__u32 hash_size);
extern void heartbeat_count(struct lb_struct *lb_hlist,__u32 hash_size,__u32 max_count);
extern void print_lb_hlist(struct lb_struct *lb_hlist,int hash_size);
extern __u32 process_message_lb(struct lb_struct *lb_hlist,__u32 hash_size,in_addr_t srcaddr,struct message *msg);
extern int is_normal(struct lb_struct *lb_hlist,__u32 id);
/*
   initialize the structure of frontend server
*/
struct topnode *
init_topnode(){
    struct topnode * traffic_serv = (struct topnode *)malloc(sizeof(struct topnode));
    if(traffic_serv == NULL){
        printf("malloc topnode struct error!\n");
        exit(1);
    }
    Pthread_mutex_init(&(traffic_serv->cur_lock),NULL);
    traffic_serv->cur = 0;
    traffic_serv->ipaddr = 0;
    traffic_serv->state = INITIAL_STATE;
    traffic_serv->node_type = MS;
    traffic_serv->hash_size = 0;
	traffic_serv->client_size = 1024;
    traffic_serv->tcp_threads = 10;
    traffic_serv->udp_threads = 1;
    traffic_serv->max_count  = 5;
    traffic_serv->lb_hlist =NULL;
    return traffic_serv;

}

/*
   read the configure file of frontend server
*/
void read_topnode_config(struct topnode *traffic_serv){
    char line[MAXLINE],key[MAXLINE],value[MAXLINE];
    //struct topnode *traffic_serv;
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
        if(!strcmp(key,"sche_type")){
            if(!strcmp(value,"ROUND_ROBIN"))
                traffic_serv->sche_type = ROUND_ROBIN;
            else if(!strcmp(value,"SRC_HASH"))
                traffic_serv->sche_type = SRC_HASH;
        }else if(!strcmp(key,"node_type")){
            if(strcmp(value,"MS")!=0){
                printf("read configure error!\n");
                exit(1);
            }
        }else if(!strcmp(key,"lbs_config")){
            read_xml_config(traffic_serv,value);
        }else if(!strcmp(key,"tcp_threads")){
            traffic_serv->tcp_threads = atoi(value);
        }else if(!strcmp(key,"udp_threads")){
            traffic_serv->udp_threads = atoi(value);
        }else if(!strcmp(key,"max_count")){
            traffic_serv->max_count = atoi(value);
        }
    }
}


/*
   the scheduling function of client request
*/
__u32 
schedule(struct topnode *traffic_serv,in_addr_t srcaddr,struct message *msg,int connfd){
    __u32 result;
    if(traffic_serv->sche_type == ROUND_ROBIN){
        __u32 cur_node;
        Pthread_mutex_lock(&(traffic_serv->cur_lock));
        cur_node = traffic_serv->cur;
        while(!is_normal(traffic_serv->lb_hlist,traffic_serv->cur)){
            traffic_serv->cur = (traffic_serv->cur +1)% traffic_serv->hash_size;
            if(!is_normal(traffic_serv->lb_hlist,traffic_serv->cur))
                continue;
            if(cur_node ==traffic_serv->cur){
                printf("no server to service!\n");
                Pthread_mutex_unlock(&(traffic_serv->cur_lock));
                printf("release the lock!\n");
                exit(1);
            }
        }
        cur_node = traffic_serv->cur;
//        printf("cur_node = %d\n",cur_node);
        traffic_serv->cur = (traffic_serv->cur +1)% traffic_serv->hash_size;
        Pthread_mutex_unlock(&(traffic_serv->cur_lock));
  //      printf("try %d \n",cur_node);
        result =   round_robin_scheduling(traffic_serv->lb_hlist,cur_node,msg,connfd);

        if(result ==-2){
            printf("try anther backend server!\n");
            schedule(traffic_serv,srcaddr,msg,connfd);
        }else if(result <0){
            printf("server fault!\n");
            msg->msgtype =TCP_SERV_FAULT;
            Send(connfd,msg,sizeof(struct message),0);
            return 0;
        }
    }
    else if(traffic_serv->sche_type == SRC_HASH)
        return srcaddr_hash_scheduling(traffic_serv->lb_hlist,srcaddr,traffic_serv->hash_size,msg,connfd);
    return result;
}

/*
   the heartbeat message handler
*/
void 
receive_heartbeat(struct topnode *traffic_serv,__u32 id){
    get_heartbeat(traffic_serv->lb_hlist,id,traffic_serv->hash_size);
}

/*************process tcp messages ***************/


/*
   the tcp message handler of frontend machine
*/
int receive_tcp_message(struct topnode* traffic_serv,int connfd){
    char buf[PACKET_LEN];
    int len;
    struct message *msg;
    struct in_addr addr;
	int srcaddr;
    for(;;){
        len = Recv(connfd,(void*)buf,PACKET_LEN,0);
        if(len <=0){
            //printf("the connection was closed by %s!\n",inet_ntoa(addr));
            return 0;
        }
        msg = (struct message*)buf;
		srcaddr = addr.s_addr = (msg->info).srcNode;
        switch(msg->msgtype){
            case TCP_ACTIVATION:
                printf("receive TCP_ACTIVATION message from %s\n",inet_ntoa(addr));
                len = process_message_lb(traffic_serv->lb_hlist,traffic_serv->hash_size,srcaddr,msg);
                if(len !=0){
                    Send(connfd,msg,len,0);
                }
                break;
            case TCP_SERV:
                printf("receive TCP_SERV message!\n");
                len = schedule(traffic_serv,srcaddr,msg,connfd);
                if(len ==-1){
                    msg->msgtype = TCP_SERV_FAULT;
                    Send(connfd,msg,sizeof(struct message),0);
                }
                break;
            case TCP_SERV_REPLY:
                printf("receive TCP_SERV_REPLY message!\n");
                len = sizeof(struct message);
                if(len !=0){
                    Send(connfd,msg,len,0);
                }
                break;
            default:
                break;
        }
    }
}

/*
   the start entry of every tcp thread

void *
tcp_thread(void *arg){
    struct topnode *traffic_serv  = (struct topnode *)arg;
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    for(;;){
        clilen = sizeof(struct sockaddr_in);
        Pthread_mutex_lock(&tcp_sockfd_lock);
        connfd = Accept(tcp_listenfd,(struct sockaddr*)&cliaddr,&clilen);
        Pthread_mutex_unlock(&tcp_sockfd_lock);
        //printf("the message was from %s\n",inet_ntoa(cliaddr.sin_addr));
        receive_tcp_message(traffic_serv,connfd,cliaddr.sin_addr.s_addr);
        close(connfd);
    }
}
*/


/*
   the tcp service program entry,this function will create a server socket and create many threads to process tcp message
*/

void* 
monitor_process(void * arg){
	int tcp_listenfd;
    int optval=1;
    struct topnode *traffic_serv = (struct topnode *)arg;
    struct sockaddr_in serv;
    bzero(&serv,sizeof(struct sockaddr_in));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(TCP_PORT);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_listenfd = socket(AF_INET,SOCK_STREAM,0);
    int err = setsockopt(tcp_listenfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(err){
        perror("setsockopt error");
        exit(1);
    }
    if(tcp_listenfd<0){
        perror("socket error");
        exit(1);
    }
    if(bind(tcp_listenfd,(struct sockaddr*)&serv,sizeof(struct sockaddr_in))<0){
        perror("bind error");
        exit(1);
    }
    if(listen(tcp_listenfd,LISTENQ)<0){
        perror("listen error");
        exit(1);
    }
	epoll_tcpserver(traffic_serv,tcp_listenfd);
    return NULL;
}

void epoll_tcpserver(struct topnode * traffic_serv,int listenfd){
	int epfd,i;
	struct sockaddr_in cliaddr;
	int clilen = sizeof(cliaddr);
	struct epoll_event ev;
	struct epoll_event *events =  (struct epoll_event*)malloc(sizeof(struct epoll_event) * traffic_serv->client_size);
	epfd = epoll_create(10);
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev)<0){
		perror("epoll_ctl error");
		return ;
	}
	int nready;
	for(;;){ // try blocking model
		nready = epoll_wait(epfd,events,traffic_serv->client_size,-1);
		if(nready < 0){
			perror("epoll_wait error");
			return ;
		}
		for(i=0;i<nready;i++){
			if(events[i].data.fd == listenfd){	//have new connection
				int connfd = accept(listenfd,(struct sockaddr*) &cliaddr,(socklen_t *)&clilen);
				if(connfd <0){
					perror("accept error");
					continue;
				}
				ev.data.fd = connfd;
				ev.events = EPOLLIN;
				if(epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev)<0){
					perror("epoll error");
					continue;
				}

			}else if(events[i].events & EPOLLIN){		// new data
				int connfd = events[i].data.fd;
				int flag = receive_tcp_message(traffic_serv,connfd);
				if(flag ==0){
					printf("connection over!\n");
					close(connfd);
					epoll_ctl(epfd,EPOLL_CTL_DEL,connfd,events+i);
				}
				
			}else if(events[i].events & EPOLLERR){
				printf("connection error\n");
				close(events[i].data.fd);
				epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,events+i);

			}// for else if

		}//for for(i=;

	}//for for(;;)


	return ;
}

/***************process udp message****************/
/*
   heartbeat message receiving and processing functions
*/
void*
heartbeat_process(void *arg){
    struct topnode *traffic_serv = (struct topnode *)arg;
    struct sockaddr_in servaddr;
    struct message msg;
    int udp_sockfd;
    bzero(&servaddr,sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(UDP_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(udp_sockfd<0){
        perror("socket error");
        exit(1);
    }
    if(bind(udp_sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        perror("bind error");
        exit(1);
    }
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    int n;
    for(;;){
        memset(&msg,'\0',sizeof(struct message));
        n = Recvfrom(udp_sockfd,&msg,sizeof(struct message),0,(struct sockaddr*)&cliaddr,&clilen);
        if(n != sizeof(struct message)){
            printf("receive error!\n");
            continue;
        }
    //    printf("Get heartbeat message from %s\n",inet_ntoa(cliaddr.sin_addr));
        if(msg.msgtype ==UDP_HEARTBEAT){
            get_heartbeat(traffic_serv->lb_hlist,msg.child_id,traffic_serv->hash_size);
        }
    }
    return NULL;
}

/*
   print the image of server
*/
void 
print_lb_tree(struct topnode *traffic_serv){
    struct in_addr ip;
    ip.s_addr = traffic_serv->ipaddr;
    printf("%s\n",inet_ntoa(ip));
    print_lb_hlist(traffic_serv->lb_hlist,traffic_serv->hash_size);
}

/*
   the function check the heartbeat count
*/
void*
check_children_alive(void *arg){
    struct topnode *traffic_serv = (struct topnode *)arg;
    printf("heartbeat count start !\n");
    while(1){
        heartbeat_count(traffic_serv->lb_hlist,traffic_serv->hash_size,traffic_serv->max_count);
        sleep(HEARTBEAT_RATE);
    }
    return NULL;
}

/*
    the main function that start the service of frontend function
*/
void 
topnode_start(){
    pthread_t tid;
    struct topnode * traffic_serv;

    traffic_serv = init_topnode();
    read_topnode_config(traffic_serv);
    print_lb_tree(traffic_serv);
    //monitor_process(traffic_serv);
    Pthread_create(&tid,NULL,&check_children_alive,(void*)traffic_serv);
    Pthread_create(&tid,NULL,&monitor_process,(void*)traffic_serv);
    Pthread_create(&tid,NULL,&heartbeat_process,(void*)traffic_serv);
    //Pthread_create(&tid,NULL,&serv_process,(void*)traffic_serv);
    for(;;)
        pause();
}

/*
   the function the process xml file
*/
__u32 
parse_xml(xmlDocPtr doc,xmlNodePtr cur,struct topnode *traffic_serv){
    in_addr_t node[LB_NUM];
    //char ip[32];
    int num =0;
    xmlChar *value;
    if(cur!= NULL){
        if(!xmlStrcmp(cur->name,(const xmlChar*)"node")){
            value = xmlGetProp(cur,(const xmlChar *)"ip");
            printf("ip = %s\n",value);
            traffic_serv->ipaddr = inet_addr((char*)value);
            xmlFree(value);
        }
    }
    cur = xmlFirstElementChild(cur);
    if(cur !=NULL){
        if(!xmlStrcmp(cur->name,(const xmlChar*)"children")){
            cur = xmlFirstElementChild(cur);
            while(cur!=NULL){
                if(!xmlStrcmp(cur->name,(const xmlChar*)"node")){
                    value = xmlGetProp(cur,(const xmlChar *)"ip");
                    printf("id  = %d,ip = %s\n",num,value);
                    node[num++] = inet_addr((char*)value);
                    xmlFree(value);
                    cur =  xmlNextElementSibling(cur);
                }
            }
         
        }
    }
    traffic_serv->lb_hlist = init_lb_hlist(node,num);
    traffic_serv->hash_size = num;
    printf("hash_size = %d\n",traffic_serv->hash_size);
    return 0;

}

/*
   the function that read xml configure file
*/
__u32
read_xml_config(struct topnode  *traffic_serv,char *pathname){
    xmlDocPtr doc;
    xmlNodePtr cur;
    doc = xmlParseFile(pathname);
    if(doc == NULL){
        err_quit("Document not parsed successfully!\n");
    }
    cur = xmlDocGetRootElement(doc);
    if(cur ==NULL){
        printf("empty document!\n");
        xmlFreeDoc(doc);
        exit(1);
    }
    if(xmlStrcmp(cur->name,(const xmlChar*)"node")){
        printf("document of the wrong type,root node != node!\n");
        xmlFreeDoc(doc);
        exit(1);
    }
    parse_xml(doc,cur,traffic_serv);
    //lb_hlist = init_lb_hlist(n);
    return 0;
}
