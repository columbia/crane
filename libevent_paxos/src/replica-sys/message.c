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
    memset(ping_msg, 0, PING_REQ_SIZE);
    if(NULL==ping_msg){
        goto build_ping_req_exit;
    }
    ping_msg->header.type = PING_REQ;
    ping_msg->header.data_size = PING_REQ_SIZE-SYS_MSG_HEADER_SIZE;
    ping_msg->node_id = node_id;
    ping_msg->view.view_id = cur_view->view_id;
    ping_msg->view.leader_id = cur_view->leader_id;
    ping_msg->view.req_id = cur_view->req_id;
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
    ping_msg->view.req_id = cur_view->req_id;
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

void* build_lele_msg(node_id_t node_id,lele_mod* mod,lele_msg_type type,void* arg){
    DEBUG_ENTER;
    lele_msg* corr_msg = arg;
    leader_election_msg* ret_msg = (leader_election_msg*)
        malloc(LEADER_ELECTION_MSG_SIZE);
    ret_msg->header.type = LEADER_ELECTION_MSG;
    ret_msg->header.data_size = LELE_MSG_SIZE;
    ret_msg->vc_msg.node_id = node_id;
    ret_msg->vc_msg.next_view = mod->next_view;
    if(NULL!=ret_msg){
        switch(type){
            case LELE_PREPARE:
                ret_msg->vc_msg.type = LELE_PREPARE;
                ret_msg->vc_msg.pnum = mod->next_pnum;
                ret_msg->vc_msg.content = -1;
                ret_msg->vc_msg.p_pnum = -1;
                break;
            case LELE_PREPARE_ACK:
                ret_msg->vc_msg.type = LELE_PREPARE_ACK;
                ret_msg->vc_msg.pnum = mod->acceptor.accepted_pnum;
                ret_msg->vc_msg.content = mod->acceptor.content;
                ret_msg->vc_msg.p_pnum = mod->acceptor.highest_seen_pnum;
                break;
            case LELE_ACCEPT:
                ret_msg->vc_msg.type = LELE_ACCEPT;
                ret_msg->vc_msg.pnum = corr_msg->pnum;
                ret_msg->vc_msg.content = corr_msg->content;
                ret_msg->vc_msg.p_pnum= -1;
                break;
            //caution
            case LELE_ACCEPT_ACK:
                ret_msg->vc_msg.type = LELE_ACCEPT_ACK;
                ret_msg->vc_msg.pnum = mod->acceptor.accepted_pnum;
                ret_msg->vc_msg.content = mod->acceptor.content;
                ret_msg->vc_msg.last_req = -1;
                // this should be fixed by the outside func
                break;
            case LELE_ANNOUNCE:
                ret_msg->vc_msg.type = LELE_ANNOUNCE;
                ret_msg->vc_msg.pnum = corr_msg->pnum;
                ret_msg->vc_msg.content = corr_msg->content;
                ret_msg->vc_msg.last_req = corr_msg->last_req;
                break;
            case LELE_HIGHER_NODE:
                break;
            case LELE_LAGGED:
                ret_msg->vc_msg.type = LELE_LAGGED;
                ret_msg->vc_msg.next_view = corr_msg->next_view;
                ret_msg->vc_msg.content = corr_msg->content;
            default:
                break;
        }
    }
    DEBUG_LEAVE;
    return ret_msg;
}
