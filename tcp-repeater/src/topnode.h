#ifndef _TOPNODE_H
#define _TOPNODE_H

#include "../include/transfer.h"
#include "../include/message.h"

/*
    the struct of front end 
*/
typedef struct topnode{
    pthread_mutex_t cur_lock;       //the lock of cur
    __u32 cur;                      //the id of backend machine that current service for round-robin scheduling
    in_addr_t ipaddr;               //the ip of frontend machine
    __u32 state;                    //the state of frontend machine
    __u16 node_type;                //the type of frontend machine,which is MS
    __u32 sche_type;                //the scheduling strategy
    __u32 tcp_threads;              //the number of threads used for maintenance
    __u32 udp_threads;              //the number of thread used for heartbeat
    __u32 max_count;                //maxinum heart rate count,more than that the machine is fault
    struct lb_struct *lb_hlist;     //the struct of backend machine
    __u32 hash_size;                //the number of backend machine
}topnode;


/*
   initialize the structure of frontend machine
*/
struct topnode *init_topnode();
/*
   read the configure file of frontend machine
*/
void read_topnode_config(struct topnode *traffic_serv);
/*
    configure the backend server in frontend server
*/
__u32 read_xml_config(struct topnode  *traffic_serv,char *pathname);
/*
   start the service of frontend machine
*/
void topnode_start();
/*
   print the image of server
*/
void print_lb_tree(struct topnode *traffic_serv);
/*
    scheduling function for client request  
*/
in_addr_t schedule(struct topnode *traffic_serv,in_addr_t srcaddr,struct message *msg,int connfd);
/*
   heartbeat message handler
*/
void receive_heartbeat(struct topnode *traffic_serv,__u32 node_id);
#endif
