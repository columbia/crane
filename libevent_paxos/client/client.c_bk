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

#include "../src/include/replica-sys/message.h"
#include "../src/include/util/common-header.h"
#include <unistd.h>
#include <time.h> /* 
typedef struct request_submit_msg_t{
    sys_msg_header header; 
    char data[0];
}__attribute__((packed))req_sub_msg;
#define REQ_SUB_SIZE(M) (((M)->header->data_size)+sizeof(req_sub_msg))
 */

int main ( int argc, char *argv[] )
{
    char* server_address=NULL;
    int server_port;
    int c;
    int node_id=9;
    int repeat_time=0;
    while((c=getopt(argc,argv,"n:s:p:r:"))!=-1){
        switch(c){
            case 'n':
                node_id = atoi(optarg);
                break;
            case 's':
                server_address = optarg;
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case 'r':
                repeat_time= atoi(optarg);
                break;
            case '?':
                fprintf(stderr,"Option -%c needs requires an argument\n",optopt);
                break;
            default:
                fprintf(stderr,"Unknown Option %c\n",c);
                goto main_exit_error;
        }
    }
    if(argc<9 || optind>argc){
        fprintf(stderr,"Parameter Error\n");
        goto main_exit_error;
    }
    fprintf(stderr,"the node id is %d\n",node_id);
    fprintf(stderr,"the server address is %s\n",server_address);
    fprintf(stderr,"the server address is %d\n",server_port);
    struct sockaddr_in server_sock_addr;
    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_port = htons(server_port);
    inet_pton(AF_INET,server_address,&server_sock_addr.sin_addr);
    int my_socket = socket(AF_INET,SOCK_STREAM,0);
    if(connect(my_socket,(struct sockaddr*)&server_sock_addr,sizeof(server_sock_addr))!=0){
        fprintf(stderr,"%s",strerror(errno));
        goto main_exit_error;
    }

    int ret;
    req_sub_msg* request = (req_sub_msg*)malloc(SYS_MSG_HEADER_SIZE+13);
    srand(time(NULL)+node_id);
    for(int index=0;index<1000;index++){
        int s_time = 20*((double)rand()/(double)RAND_MAX);
        printf("sleep time is %u\n",s_time);
        usleep(s_time);
        request->header.type = REQUEST_SUBMIT;
        request->header.data_size = 13;
        request->data[0] = 'c';
        request->data[1] = 'l';
        request->data[2] = 'i';
        request->data[3] = 'e';
        request->data[4] = 'n';
        request->data[5] = 't';
        request->data[6] = node_id+'0';
        request->data[7] = ':';
        request->data[8] = 's';
        request->data[9] = 'e';
        request->data[10] = 'n';
        request->data[11] = 'd';
        request->data[12] = '\0';

        if((ret=send(my_socket,request,REQ_SUB_SIZE(request),0))<0){
            goto main_exit_error;
        }
        printf("send message %s\n",request->data);
    }
    return EXIT_SUCCESS;
main_exit_error:
    return EXIT_FAILURE;
}				/* ----------  end of function main  ---------- */
