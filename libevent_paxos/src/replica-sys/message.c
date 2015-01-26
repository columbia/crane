/*
 * =====================================================================================
 *
 *       Filename:  message.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/11/2014 03:42:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../include/replica-sys/message.h"

void* build_ping_req(int node_id, view* cur_view){
    ping_req_msg* ping_msg = (ping_req_msg*)malloc(PING_REQ_SIZE);
    if(NULL==ping_msg){
        goto build_ping_req_exit;
    }
    ping_msg->header.type = PING_REQ;
    ping_msg->header.data_size = PING_REQ_SIZE-SYS_MSG_HEADER_SIZE;
    ping_msg->node_id = node_id;
    ping_msg->view.view_id = cur_view->view_id;
    ping_msg->view.leader_id = cur_view->leader_id;
    gettimeofday(&ping_msg->timestamp,NULL);
build_ping_req_exit:
    return ping_msg;
}
void* build_ping_ack(int node_id,view* cur_view){
    ping_ack_msg* ping_msg = (ping_ack_msg*)malloc(PING_ACK_SIZE);
    if(NULL==ping_msg){
        goto build_ping_ack_exit;
    }
    ping_msg->header.type = PING_ACK;
    ping_msg->header.data_size = PING_REQ_SIZE-SYS_MSG_HEADER_SIZE;
    ping_msg->node_id = node_id;
    ping_msg->view.view_id = cur_view->view_id;
    ping_msg->view.leader_id = cur_view->leader_id;
    gettimeofday(&ping_msg->timestamp,NULL);
build_ping_ack_exit:
    return ping_msg;
}

void* build_consensus_msg(uint32_t data_size,void* data){
    consensus_msg* con_msg = (consensus_msg*)(malloc(SYS_MSG_HEADER_SIZE+data_size));
    if(NULL==con_msg){
        goto build_consensus_msg_exit;
    }
    con_msg->header.type = CONSENSUS_MSG;
    con_msg->header.data_size = data_size;
    memcpy(con_msg->data,data,data_size);
build_consensus_msg_exit:
    return con_msg;
}
