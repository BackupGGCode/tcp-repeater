#ifndef _MESSAGE_H
#define _MESSAGE_H

//=======================================================
//General message structure:
//Can be used for the following messages:
//1.Activation message
//2.Acknowledgement of activation message
//3.Fault recover message
//4.Query message
//5.Fault query message
//=======================================================

typedef struct message{
    __u16 msgtype;        //Message type
    __u16 child_id;
    union{
        in_addr_t srcNode;    //Destination node of message
        __u32 links;
    }info;
    __u16 state;          //State of the source node

}message;

#endif
