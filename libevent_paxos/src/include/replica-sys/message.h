/*
 * =====================================================================================
 *
 *       Filename:  message.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/29/2014 03:44:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef MESSAGE_H
#define MESSAGE_H

#include "../util/common-header.h"

typedef enum sys_msg_code_t{
    PING_REQ = 0,
    PING_ACK = 1,
    REQUEST_SUBMIT = 2,
    REQUEST_SUBMIT_REPLY = 3,
    REQUEST_CHECK = 4,
    CONSENSUS_MSG = 5,
    CLIENT_SYNC_REQ = 11,
    CLIENT_SYNC_ACK = 12,
}sys_msg_code;

typedef enum req_sub_code_t{
    SUB_SUCC = 0,
    NO_LEADER = 1,
    IN_ERROR = 2,
    NO_RECORD = 3,
    ON_GOING = 4,
    FORFEITED = 5,
    FINISHED = 6,
}req_sub_code;

typedef struct sys_msg_header_t{
    sys_msg_code type;
    size_t data_size;
}sys_msg_header;
#define SYS_MSG_HEADER_SIZE (sizeof(sys_msg_header))

typedef struct ping_req_msg_t{
    sys_msg_header header; 
    int node_id;
    view view;
    struct timeval timestamp;
}__attribute__((packed))ping_req_msg;
#define PING_REQ_SIZE (sizeof(ping_req_msg))

typedef struct ping_ack_msg_t{
    sys_msg_header header; 
    int node_id;
    view view;
    struct timeval timestamp;
}__attribute__((packed))ping_ack_msg;
#define PING_ACK_SIZE (sizeof(ping_ack_msg))

typedef struct request_submit_msg_t{
    sys_msg_header header; 
    char data[0];
}__attribute__((packed))req_sub_msg;
#define REQ_SUB_SIZE(M) (((M)->header.data_size)+sizeof(req_sub_msg))

typedef struct request_submit_reply_msg_t{
    sys_msg_header header; 
    req_sub_code code;
    view_stamp req_id;
}__attribute__((packed))req_sub_rep_msg;
#define REQ_SUB_REP_SIZE (sizeof(req_sub_rep_msg))

typedef struct request_check_msg_t{
    sys_msg_header header; 
    view_stamp req_id;
}__attribute__((packed))req_check_msg;
#define REQ_CHECK_SIZE (sizeof(req_check_msg))

typedef struct client_sync_req_msg_t{
    sys_msg_header header; 
}__attribute__((packed))client_sync_req_msg;
#define CLIENT_SYNC_REQ_SIZE (sizeof(client_sync_req_msg))

typedef struct client_sync_ack_msg_t{
    sys_msg_header header; 
    view cur_view;
}__attribute__((packed))client_sync_ack_msg;
#define CLIENT_SYNC_ACK_SIZE (sizeof(client_sync_ack_msg))

typedef struct consensus_msg_t{
    sys_msg_header header;
    char data[0];
}__attribute__((packed))consensus_msg;
#define CONSENSUS_MSG_SIZE(M) (sizeof(sys_msg_header)+M->header.data_size)

//sys_msg* package_sys_msg(sys_msg_code,int,void*);

void* build_consensus_msg(uint32_t,void*);
void* build_ping_req(int,view*);
void* build_ping_ack(int,view*);
void* build_client_sync_ack(view*);
void* build_request_submit_reply_msg(req_sub_code,view_stamp*);

#endif
