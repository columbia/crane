/*
 * =====================================================================================
 *
 *       Filename:  client.c *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/26/2014 10:19:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../src/include/rsm-interface.h"
#include "../src/include/util/common-header.h"
#include <unistd.h>
#include <time.h> /* 

typedef struct request_submit_msg_t{
    sys_msg_header header; 
    char data[0];
}__attribute__((packed))req_sub_msg;
#define REQ_SUB_SIZE(M) (((M)->header->data_size)+sizeof(req_sub_msg))
 */

static int max_fd = 256;
static int max_buf = 2048;
static char ack[] = "ack:msg";

int main ( int argc, char *argv[] )
{
    int max_client[max_fd];
    int new_socket = -1;
    char buf[max_buf];
    memset(max_client,0,max_fd*sizeof(int));
    char* server_address=NULL;
    int server_port;
    int c;
    while((c=getopt(argc,argv,"s:p:"))!=-1){
        switch(c){
            case 's':
                server_address = optarg;
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case '?':
                fprintf(stderr,"Option -%c needs requires an argument\n",optopt);
                break;
            default:
                fprintf(stderr,"Unknown Option %c\n",c);
                goto main_exit_error;
        }
    }

//    fprintf(stderr,"the node id is %d\n",node_id);
//    fprintf(stderr,"the server address is %s\n",server_address);
//    fprintf(stderr,"the server address is %d\n",server_port);

    fd_set sock_set;
    struct sockaddr_in server_sock_addr;
    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_port = htons(server_port);
    inet_pton(AF_INET,server_address,&server_sock_addr.sin_addr);


    int my_socket = socket(AF_INET,SOCK_STREAM,0);

    if(bind(my_socket,(struct sockaddr*)&server_sock_addr,sizeof(server_sock_addr))!=0){
        fprintf(stderr,"%s",strerror(errno));
        goto main_exit_error;
    }

    if(listen(my_socket,10)!=0){
        fprintf(stderr,"%s",strerror(errno));
        goto main_exit_error;
    }
    FD_SET(my_socket,&sock_set);
    while(1){
        FD_ZERO(&sock_set);
        FD_SET(my_socket,&sock_set);
        for(int i=0;i<max_fd;i++){
            if(max_client[i]!=0){
                FD_SET(max_client[i],&sock_set);
            }
        }
        if(select(max_fd+1,&sock_set,NULL,NULL,NULL)<0){
            fprintf(stderr,"select error.\n");
        }
        if(FD_ISSET(my_socket,&sock_set)){
            if((new_socket = accept(my_socket,NULL,0))<0){
                fprintf(stderr,"accept error.\n");
                goto main_exit_error;
            }
            fprintf(stderr,"a new socket is here.\n");
            for(int i = 0;i<max_fd;i++){
                if(!max_client[i]){
                    max_client[i] = new_socket;
                    new_socket = -1;
                    break;
                }
            }
        }
        for(int i=0;i<max_fd;i++){
            if(max_client[i]!=0 && FD_ISSET(max_client[i],&sock_set)){
                if(read(max_client[i],buf,max_buf)==0){
                    close(max_client[i]);
                    max_client[i] = 0;
                }else{
                    send(max_client[i],ack,strlen(ack),0);
                }
            }
        }
    }

    return EXIT_SUCCESS;
main_exit_error:
    return EXIT_FAILURE;
}				/* ----------  end of function main  ---------- */
