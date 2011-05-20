#include "../include/transfer.h"
#include "../include/nodetype.h"
extern void realserv_start();
extern void topnode_start();

/*
   start the server
*/
int main(){ 
    char line[MAXLINE],key[MAXLINE],value[MAXLINE];
    //struct topnode *traffic_serv;
    signal(SIGPIPE,SIG_IGN);
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
        if(!strcmp(key,"node_type")){
            if(!strcmp(value,"MS")){
                topnode_start();
            }else if(!strcmp(value,"LB")){
                realserv_start();
            }
            break;
        }
    }
    return 0;
}



