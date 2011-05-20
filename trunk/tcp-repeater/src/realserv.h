#ifndef _REALSERV_H
#define _REALSERV_H

#include "../include/transfer.h"

/*
   the structure of backend machine
*/
typedef struct childnode{
    in_addr_t father_ip;            //the ip of frontend machine
    __u16 id;                       //the id of current backend machine
    pthread_mutex_t lock;           //the lock of this structure,main for state 
    __u16 state;                    //the state of current backend machine
    __u16 tcp_threads;              //the number of threads used for maintain and service 
    __u32 links;                    //the links of current machine
    __u32 max_links;                //the max links that current machine can afford
}childnode;

/*
   initialize the backend machine structure
*/
struct childnode *init_childnode();

/*
    read the configure file of backend machine
*/
void read_realserv_config(struct childnode *node);

/*
   the key function that start the service of backend machine
*/
void realserv_start();

/*
   tcp message handler in backend
*/
void recv_tcp_message(struct childnode *child,int connfd,in_addr_t srcaddr);

#endif
