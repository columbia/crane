/*
 * =====================================================================================
 *
 *       Filename:  server.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/23/2014 03:49:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef NODE_H
#define NODE_H 
#include "../util/common-header.h"
#include "../consensus/consensus.h"
#include "message.h"
#include "../db/db-interface.h"
#include "./replica.h"

typedef struct peer_t{
    int peer_id;
    int active;

    int sock_id;
    struct sockaddr_in* peer_address;
    size_t sock_len;

    struct node_t* my_node;

    struct bufferevent* my_buff_event;
    struct event_base* base;
    struct event* reconnect; // reconnect the peer
}peer;

typedef enum node_state_code_t{
    NODE_ACTIVE=0, // normal state
    NODE_INLELE=1, //in leader election, in this stage, we are still finding a new leader, and this state can be interrupted by 
    NODE_POSTLELE=2,// a new leader is elected out, and we must wait for some post activities from the new leader, and in this stage, the node won't receive any ping msg, that is, the leader election process cannot be canceled now.
    NODE_WAITFORSYNC=3, //lagged behind,wait for ping msg
}node_state_code;

typedef struct node_config_t{
    struct timeval reconnect_timeval;
    struct timeval ping_timeval;
    struct timeval expect_ping_timeval;
    struct timeval make_progress_timeval;
}node_config;


typedef void (*user_cb)(size_t data_size,void* data,void* arg);

typedef void (*msg_handler)(struct node_t*,struct bufferevent*,size_t);

typedef struct node_t{

    node_id_t node_id;

    int stat_log;
    int sys_log;

    view cur_view;
    view_stamp highest_to_commit;
    view_stamp highest_committed;
    view_stamp highest_seen;
    //leader election
    node_state_code state;
    lele_mod* election_mod; 
    //consensus component
    struct consensus_component_t* consensus_comp;

    // replica group
    struct sockaddr_in my_address;
    uint32_t group_size;
    peer* peer_pool;

    //config
    node_config config;
    // libevent part
    struct evconnlistener* listener;
    struct event_base* base;

    msg_handler msg_cb;
    struct event* signal_handler;
    struct event* ev_leader_ping;
    struct event* ev_expect_ping;
    struct event* ev_make_progress;
    struct timeval last_ping_msg;

    //databse part
    char* db_name;
    db* db_ptr;
    FILE* sys_log_file;

    //database* my_db
}node;

//struct node_t* system_initialize(int node_id,const char* start_mode,const char* config_path,const char* log_path,int deliver_mode,void(*user_cb)(size_t data_size,void* data,void* arg),void* db_ptr,void* arg);
//void system_run(struct node_t* replica);
//void system_exit(struct node_t* replica);

#endif
