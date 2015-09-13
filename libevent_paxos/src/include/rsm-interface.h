/*
 * =====================================================================================
 *
 *       Filename:  rsm-interface.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/16/2014 03:45:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef RSM_INTERFACE_H
#define RSM_INTERFACE_H
#include <unistd.h>
#include <stdint.h>

typedef enum client_msg_code_t{
    C_SEND_WR = 0,
    C_SEND_RD = 1,
    //further structure is reserved for potential expansion
}client_msg_code;

typedef struct client_msg_header_t{
    client_msg_code type;
    size_t data_size;
}client_msg_header;
#define CLIENT_MSG_HEADER_SIZE (sizeof(client_msg_header))

typedef struct client_msg_t{
    client_msg_header header;
    char data[0];
}__attribute__((packed))client_msg;
#define CLIENT_MSG_SIZE(M) (sizeof(client_msg)+((M)->header.data_size))

struct proxy_node_t;

struct proxy_node_t* proxy_init(int64_t node_id,const char* start_mode,const char* config_path,
        const char* log_path,int fake_mode);
void proxy_run(struct proxy_node_t* proxy);
void proxy_exit(struct proxy_node_t* proxy);

#endif
