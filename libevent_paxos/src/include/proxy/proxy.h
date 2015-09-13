/*
 * =====================================================================================
 *
 *       Filename:  proxy.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/14/2014 11:20:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 * *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * ===================================================================================== */ 

#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"
#include "../replica-sys/replica.h"
#include "uthash.h"


// hash_key type def
typedef uint64_t hk_t;
typedef uint64_t sec_t;
typedef uint32_t nid_t;
typedef uint16_t nc_t;
typedef uint64_t counter_t;

// record number
typedef uint64_t rec_no_t;
typedef uint64_t flag_t;

struct proxy_node_t;


// hash table of socket pair
typedef struct socket_pair_t{
    hk_t key;
    counter_t counter;
    struct proxy_node_t* proxy;
    struct bufferevent* p_s;
    struct bufferevent* p_c;
    // hash pair handle
    UT_hash_handle hh;
}socket_pair;

typedef struct proxy_address_t{
    struct sockaddr_in p_addr;
    size_t p_sock_len;
    struct sockaddr_in c_addr;
    size_t c_sock_len;
    struct sockaddr_in s_addr;
    size_t s_sock_len;
}proxy_address;


typedef struct proxy_node_t{
    // socket pair
    nid_t node_id; 
    nc_t pair_count;
    socket_pair* hash_map;
    proxy_address sys_addr;
    int fake;

    // log option
    int ts_log;
    int sys_log;
    int stat_log;
    int req_log;

    // libevent part
    struct event_base* base;
    struct evconnlistener* listener;
    struct timeval recon_period;
    //signal handler
    struct event* sig_handler;
    // DMT part, option
    int sched_with_dmt; 

    // in the loop of libevent logical, we must give a way to periodically
    // invoke our actively working function
    struct event* do_action;
    struct timeval action_period;
    db_key_type cur_rec;
    db_key_type highest_rec;
    // consensus module
    pthread_t sub_thread;
    pthread_t p_self;
    struct node_t* con_node;
    struct bufferevent* con_conn;
    struct event* re_con;
    FILE* req_log_file;
    FILE* sys_log_file;
    char* db_name;
    db* db_ptr;
    // for call back of the thread;
    pthread_mutex_t lock;

}proxy_node;

typedef enum proxy_action_t{
    P_CONNECT=0,
    P_SEND=1,
    P_CLOSE=2,
    P_DMT_CLKS =3,
}proxy_action;

typedef struct proxy_msg_header_t{
    proxy_action action;
    struct timeval received_time;
    struct timeval created_time;
    hk_t connection_id;
    counter_t counter;
}proxy_msg_header;
#define PROXY_MSG_HEADER_SIZE (sizeof(proxy_msg_header))

typedef struct proxy_connect_msg_t{
    proxy_msg_header header;
}proxy_connect_msg;
#define PROXY_CONNECT_MSG_SIZE (sizeof(proxy_connect_msg))

typedef struct proxy_send_msg_t{
    proxy_msg_header header;
    size_t data_size;
    char data[0];
}__attribute__((packed))proxy_send_msg;
#define PROXY_SEND_MSG_SIZE(M) (M->data_size+sizeof(proxy_send_msg))

typedef struct proxy_close_msg_t{
    proxy_msg_header header;
}proxy_close_msg;
#define PROXY_CLOSE_MSG_SIZE (sizeof(proxy_close_msg))


#define MY_HASH_SET(value,hash_map) do{ \
    HASH_ADD(hh,hash_map,key,sizeof(hk_t),value);}while(0)

#define MY_HASH_GET(key,hash_map,ret) do{\
 HASH_FIND(hh,hash_map,key,sizeof(hk_t),ret);}while(0) 

#endif
