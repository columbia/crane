/*
 * =====================================================================================
 *
 *       Filename:  proxy.c
 * *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/14/2014 11:19:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "../include/proxy/proxy.h"
#include "../include/config-comp/config-proxy.h"
#include "../include/replica-sys/message.h"
#include "../include/replica-sys/node.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "tern/runtime/paxos-op-queue.h"
 #include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>

static void* hack_arg=NULL;

//helper function
static hk_t gen_key(nid_t,nc_t,sec_t);

// consensus callback
static void update_state(size_t,void*,void*);
static void fake_update_state(size_t,void*,void*);
static void usage();

static void proxy_do_action(int fd,short whaDB_t,void* arg);
static void do_action_to_server(size_t data_size,void* data,void* arg);
static void do_action_connect(size_t data_size,void* data,void* arg);
static void do_action_send(size_t data_size,void* data,void* arg);
static void do_action_close(size_t data_size,void* data,void* arg);
static void do_action_give_dmt_clocks(size_t data_size,void* data,void* arg);

static void proxy_on_accept(struct evconnlistener* listener,evutil_socket_t
        fd,struct sockaddr *address,int socklen,void *arg);
static void consensus_on_event(struct bufferevent* bev,short ev,void* arg);
static void consensus_on_read(struct bufferevent*,void*);
static void connect_consensus(proxy_node*);
static void reconnect_on_timeout(int fd,short what,void* arg);

 /* For time bubble mechanism. */
static void proxy_on_read_timebubble_req(struct bufferevent *bev, void *arg);
static void proxy_on_timebubble_conn_err(struct bufferevent *bev, short events, void *arg);
static void proxy_on_accept_timebubble_conn(struct evconnlistener* listener, evutil_socket_t
        fd,struct sockaddr *address,int socklen,void *arg);
static void proxy_timebubble_init(struct event_base* base, void *arg);
static void proxy_invoke_timebubble_consensus(void *arg, int timebubble_cnt);
static req_sub_msg* build_timebubble_req_sub_msg(int timebubble_cnt);


//socket pair callback function

static unsigned global_req_count=0;
static void server_side_on_read(struct bufferevent* bev,void* arg);
static void server_side_on_err(struct bufferevent* bev,short what,void* arg);
static void client_process_data(socket_pair*,struct bufferevent*,size_t);
static void client_side_on_read(struct bufferevent* bev,void* arg);
static void client_side_on_err(struct bufferevent* bev,short what,void* arg);

static req_sub_msg* build_req_sub_msg(hk_t s_key,counter_t counter,int type,size_t data_size,void* data);

static void wake_up(proxy_node* proxy);
static void real_do_action(proxy_node* proxy);

static void cross_view(proxy_node* proxy){
    proxy->cur_rec = proxy->cur_rec>>32;
    proxy->cur_rec++;
    proxy->cur_rec = proxy->cur_rec<<32;
    proxy->cur_rec++;
};

static void proxy_do_action(int fd,short what,void* arg){
    proxy_node* proxy = arg;
    SYS_LOG(proxy,"Proxy Triggers Periodically Checking Event.Now Checking Pending Requests\n");
    real_do_action(proxy);
}

static void real_do_action(proxy_node* proxy){
    request_record* data = NULL;
    size_t data_size=0;
    PROXY_ENTER(proxy);
    db_key_type cur_higest;
    pthread_mutex_lock(&proxy->lock);
    cur_higest = proxy->highest_rec;
    pthread_mutex_unlock(&proxy->lock);
    SYS_LOG(proxy,"In REAL Do Action,The Current Rec Is %lu\n",proxy->cur_rec);
    SYS_LOG(proxy,"In REAL Do Action,The Highest Rec Is %lu\n",cur_higest);
    FILE* output = NULL;
    if(proxy->req_log){
        output = proxy->req_log_file;
    }
    while(proxy->cur_rec<=cur_higest){
        SYS_LOG(proxy,"In REAL Do Action,We Execute Rec %lu\n",proxy->cur_rec);
        data = NULL;
        data_size = 0;
        retrieve_record(proxy->db_ptr,sizeof(db_key_type),&proxy->cur_rec,&data_size,(void**)&data);
        if(NULL==data){
            cross_view(proxy);
        }else{
            if(output!=NULL){
                fprintf(output,"Request:%ld:",proxy->cur_rec);
            }
            do_action_to_server(data->data_size,data->data,proxy);
            proxy->cur_rec++;
        }
    }
    //evtimer_add(proxy->do_action,&proxy->action_period);
    PROXY_LEAVE(proxy);
}

static void update_state(size_t data_size,void* data,void* arg){
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    //SYS_LOG(proxy,"In Update State,The Current Rec Is %lu.\n",*((db_key_type*)(data)));
    db_key_type* rec_no = data;
    //pthread_mutex_lock(&proxy->lock);
    proxy->highest_rec = (proxy->highest_rec<*rec_no)?*rec_no:proxy->highest_rec;
    //SYS_LOG(proxy,"In Update State,The Highest Rec Is %lu.\n",proxy->highest_rec);
    //pthread_mutex_unlock(&proxy->lock);
    wake_up(proxy);
    PROXY_LEAVE(proxy);
    return;
}

static void wake_up(proxy_node* proxy){
    //SYS_LOG(proxy,"In Wake Up,The Highest Rec Is %lu.\n",proxy->highest_rec);
    PROXY_ENTER(proxy);
    pthread_kill(proxy->p_self,SIGUSR2);
    PROXY_LEAVE(proxy);
}

