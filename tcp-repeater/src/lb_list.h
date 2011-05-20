#ifndef _LB_LIST_H
#define _LB_LIST_H

#include "../include/transfer.h"
#include "../include/message.h"

/*
   the backend structure in frontend machine
*/
typedef struct lb_struct{
    pthread_mutex_t lb_lock;            //the lock of the lb_struct
    in_addr_t ipaddr;                   //the ip of backend machine
    __u32 state;                        //the state of backend machine
    __u32 count;                        //the heartbeat count of current backend machine
//    int  connfd;                        //the connected socket to current backend machine
}lb_struct;



/*
   initialize the structure of backend machine
*/
struct lb_struct *init_lb_hlist(in_addr_t node[],__u32 num);

/*
   the round_robin scheduling method
*/
int round_robin_scheduling(struct lb_struct *lb_hlist,__u32 cur,struct message *msg,int connfd);

/*
   the source address hash method
*/
int srcaddr_hash_scheduling(struct lb_struct *lb_hlist,in_addr_t srcaddr,__u32 hash_size,struct message *msg,int connfd);

/*
   the heartbeat message handler in lb_struct
*/
void get_heartbeat(struct lb_struct *lb_hlist,__u32 id,__u32 hash_size);

/*
    check the count of hearbeat
*/
void heartbeat_count(struct lb_struct *lb_hlist,__u32 hash_size,__u32 max_count);

/*
   print the ip address of backend machine
*/
void print_lb_hlist(struct lb_struct *lb_hlist,int hash_size);

/*
   process the tcp message in lb_struct 
*/
__u32 process_message_lb(struct lb_struct *lb_hlist,__u32 hash_size,in_addr_t srcaddr,struct message *msg);

/*
   check if the backend in normal state
*/
int is_normal(struct lb_struct *lb_hlist,int id);
#endif






















