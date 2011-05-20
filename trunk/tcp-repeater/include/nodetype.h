
//=============================================================
/*
This head file defines all the message types that will
be used in the CG tree evironment,all operations on 
the resources and all states of the nodes.
CopyRight (C) Allen
*/
//=============================================================

#ifndef _NODE_TYPE_H_
#define _NODE_TYPE_H_

#define UDP_HEARTBEAT           0X0001  //Heartbeat message
#define TCP_ACTIVATION          0X0002  //Activation message
#define TCP_ACTIVATION_ACK      0X0004  //Acknowledgement of Activation message
#define TCP_FAULT_RECOVER       0X0008  //Fault recover message
#define TCP_QUERY               0X0100  //Query message
#define TCP_FAULT_QUERY         0X0200  //Fault query message
#define TCP_FAULT_QUERY_ACK     0X1000
#define TCP_SERV                0X0400
#define TCP_SERV_REPLY          0X0800
#define TCP_SERV_FAULT          0X2000
#define NORMAL_STATE            0X0001  //Normal state
#define FAULT_STATE             0X0002  //Fault state
#define ACTIVATION_STATE        0X0004  //Activation state
#define INITIAL_STATE           0X0008  //Initial state


#define MS          0X0001
#define LB          0X0002

#define ROUND_ROBIN 0X0001
#define SRC_HASH    0X0002
#endif
