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

#ifndef PROXYTEMP_H 
#define PROXYTEMP_H

// hash_key type def
typedef uint64_t hk_t;
typedef uint64_t counter_t;


typedef enum proxy_action_t{
    P_CONNECT=0,
    P_SEND=1,
    P_CLOSE=2,
}proxy_action;

typedef struct proxy_msg_header_t{
    proxy_action action;
    struct timeval received_time;
    struct timeval created_time;
    // tom add 20150131
    struct timeval timestamp_0;
    struct timeval timestamp_1;
    struct timeval timestamp_2;
    // end tom add
    hk_t connection_id;
    counter_t counter;
}proxy_msg_header;
#define PROXY_MSG_HEADER_SIZE (sizeof(proxy_msg_header))

#endif
