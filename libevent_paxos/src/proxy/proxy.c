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
  // tom add 20150127
 #define _GNU_SOURCE  
#include <unistd.h>  
#include <sched.h>  
#include <assert.h>  
 //end tom
#include "../include/proxy/proxy.h"
#include "../include/config-comp/config-proxy.h"
#include "../include/replica-sys/message.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

static void* hack_arg=NULL;

//helper function
static hk_t gen_key(nid_t,nc_t,sec_t);

// consensus callback
static void update_state(int,void*,void*);
static void fake_update_state(int,void*,void*);
static void usage();

static void proxy_do_action(int fd,short whaDB_t,void* arg);
static void do_action_to_server(int data_size,void* data,void* arg);
static void do_action_connect(int data_size,void* data,void* arg);
static void do_action_send(int data_size,void* data,void* arg);
static void do_action_close(int data_size,void* data,void* arg);

static void proxy_on_accept(struct evconnlistener* listener,evutil_socket_t
        fd,struct sockaddr *address,int socklen,void *arg);
static void consensus_on_event(struct bufferevent* bev,short ev,void* arg);
static void consensus_on_read(struct bufferevent*,void*);
static void connect_consensus(proxy_node*);
static void reconnect_on_timeout(int fd,short what,void* arg);


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

typedef struct signal_comp_st{
    void* event;
    void* proxy;
}signal_comp;

//log component

//implementation

static hk_t gen_key(nid_t node_id,nc_t node_count,sec_t time){
    hk_t key = time;
    key |= ((hk_t)node_id<<52);
    key |= ((hk_t)node_count<<36);
    return key;
}

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

static void do_action_to_server(int data_size,void* data,void* arg){
    
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    proxy_msg_header* header = data;
    FILE* output = NULL;
    if(proxy->req_log){
        output = proxy->req_log_file;
    }
    struct timeval endtime;
    gettimeofday(&endtime,NULL);
    if(proxy->ts_log && output!=NULL){
        //fprintf(output,"\n");
        fprintf(output,"%lu : %lu.%06lu,%lu.%06lu,%lu.%06lu,%lu.%06lu\n",header->connection_id,
            header->received_time.tv_sec,header->received_time.tv_usec,
            header->created_time.tv_sec,header->created_time.tv_usec,
            endtime.tv_sec,endtime.tv_usec,endtime.tv_sec,endtime.tv_usec);
    }
    switch(header->action){
        case P_CONNECT:
            if(output!=NULL){
                fprintf(output,"Operation: Connects.\n");
            }
            do_action_connect(data_size,data,arg);
            break;
        case P_SEND:
            if(output!=NULL){
                //fprintf(output,"Operation: Sends data: (START):%s:(END).\n",((proxy_send_msg*)header)->data);
                fprintf(output,"Operation: Sends data.\n");
            }
            do_action_send(data_size,data,arg);
            break;
        case P_CLOSE:
            if(output!=NULL){
                fprintf(output,"Operation: Closes.\n");
            }
            do_action_close(data_size,data,arg);
            break;
        default:
            break;
    }
    if(NULL!=output){
        fprintf(output,"\n");
    }
    if(output!=NULL){
        fflush(output);
    }
    PROXY_LEAVE(proxy);
    return;
}
// when we have seen a connect method;
static void do_action_connect(int data_size,void* data,void* arg){
    
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    proxy_msg_header* header = data;
    socket_pair* ret = NULL;
    MY_HASH_GET(&header->connection_id,proxy->hash_map,ret);
    if(NULL==ret){
        ret = malloc(sizeof(socket_pair));
        memset(ret,0,sizeof(socket_pair));
        ret->key = header->connection_id;
        ret->counter = 0;
        ret->proxy = proxy;
        MY_HASH_SET(ret,proxy->hash_map);
    }
    if(ret->p_s==NULL){
        ret->p_s = bufferevent_socket_new(proxy->base,-1,BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(ret->p_s,server_side_on_read,NULL,server_side_on_err,ret);
        bufferevent_enable(ret->p_s,EV_READ|EV_PERSIST|EV_WRITE);
        bufferevent_socket_connect(ret->p_s,(struct sockaddr*)&proxy->sys_addr.s_addr,proxy->sys_addr.s_sock_len);
    }else{
        debug_log("why there is an existing connection?\n");
    }
    PROXY_LEAVE(proxy);
    
    return;
}
// when we have seen a send method,and since we have seen the same request
// sequence on all the machine, then this time, we must have already set up the
// connection with the server
static void do_action_send(int data_size,void* data,void* arg){
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    proxy_send_msg* msg = data;
    socket_pair* ret = NULL;
    MY_HASH_GET(&msg->header.connection_id,proxy->hash_map,ret);
    // this is error, TO-DO:error handler
    if(NULL==ret){
        goto do_action_send_exit;
    }else{
        if(NULL==ret->p_s){
            goto do_action_send_exit;
        }else{
            bufferevent_write(ret->p_s,msg->data,msg->data_size);
        }
    }
do_action_send_exit:
    PROXY_LEAVE(proxy);
    return;
}

static void do_action_close(int data_size,void* data,void* arg){
    proxy_node* proxy = arg;
    proxy_close_msg* msg = data;
    PROXY_ENTER(proxy);
    socket_pair* ret = NULL;
    MY_HASH_GET(&msg->header.connection_id,proxy->hash_map,ret);
    // this is error, TO-DO:error handler
    if(NULL==ret){
        goto do_action_close_exit;
    }else{
        if(ret->p_s!=NULL){
            bufferevent_free(ret->p_s);
            ret->p_s = NULL;
        }
        if(ret->p_c!=NULL){
            bufferevent_free(ret->p_c);
            ret->p_c = NULL;
        }
    }
    PROXY_LEAVE(proxy);
do_action_close_exit:
    return;
}

static void update_state(int data_size,void* data,void* arg){
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

//fake update state, we take out the data directly without re-
//visit the database
static void fake_update_state(int data_size,void* data,void* arg){
    proxy_node* proxy = arg;
    proxy_msg_header* header = data;
    FILE* output = proxy->req_log_file;
    if(proxy->req_log){
        output = proxy->req_log_file;
    }
    struct timeval endtime;
    gettimeofday(&endtime,NULL);
    if(proxy->ts_log && output!=NULL){
        fprintf(output,"\n");
        fprintf(output,"%lu : %lu.%06lu,%lu.%06lu,%lu.%06lu,%lu.%06lu\n",header->connection_id,
            header->received_time.tv_sec,header->received_time.tv_usec,
            header->created_time.tv_sec,header->created_time.tv_usec,
            endtime.tv_sec,endtime.tv_usec,endtime.tv_sec,endtime.tv_usec);
    }
    switch(header->action){
        case P_CONNECT:
            if(NULL!=output){
                fprintf(output,"Operation: Connects.\n");
            }
            break;
        case P_SEND:
            if(NULL!=output){
                fprintf(output,"Operation: Sends data: (START):%s:(END)\n",
                        ((proxy_send_msg*)header)->data);
            }
            break;
        case P_CLOSE: if(NULL!=output){
                fprintf(output,"Operation: Closes.\n");
            }
            break;
        default:
            break;
    }
    
    return;
}


static void* t_consensus(void *arg){
    struct node_t* my_node = arg;
    system_run(my_node);
    system_exit(my_node);
    return NULL;
}

//public interface
void consensus_on_event(struct bufferevent* bev,short ev,void* arg){
    
    proxy_node* proxy = arg;
    if(ev&BEV_EVENT_CONNECTED){
        SYS_LOG(proxy,"Connected to Consensus.\n");
    }else if((ev & BEV_EVENT_EOF )||(ev&BEV_EVENT_ERROR)){
        int err = EVUTIL_SOCKET_ERROR();
		SYS_LOG(proxy,"%s.\n",evutil_socket_error_to_string(err));
        bufferevent_free(bev);
        proxy->con_conn = NULL;
        event_add(proxy->re_con,&proxy->recon_period);
    }
    
    return;
}

//void consensus_on_read(struct bufferevent* bev,void*);
void connect_consensus(proxy_node* proxy){
    
    proxy->con_conn = bufferevent_socket_new(proxy->base,-1,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(proxy->con_conn,NULL,NULL,consensus_on_event,proxy);
    bufferevent_enable(proxy->con_conn,EV_READ|EV_WRITE|EV_PERSIST);
    bufferevent_socket_connect(proxy->con_conn,(struct sockaddr*)&proxy->sys_addr.c_addr,proxy->sys_addr.c_sock_len);
    
    return;
}


void reconnect_on_timeout(int fd,short what,void* arg){
    // this function should also test whether it should re-set 
    // a thread to run the consensus module but currently 
    // ignore this
    connect_consensus(arg);
}


static void server_side_on_read(struct bufferevent* bev,void* arg){
    socket_pair* pair = arg;
    proxy_node* proxy = pair->proxy;
    PROXY_ENTER(proxy);
    struct evbuffer* input = bufferevent_get_input(bev);
    struct evbuffer* output = NULL;
    size_t len = 0;
    int cur_len = 0;
    void* msg = NULL;
    len = evbuffer_get_length(input);
    SYS_LOG(proxy,"There Is %u Bytes Data In The Buffer In Total.\n",
            (unsigned)len);
    // every time we just send 1024 bytes data to the client
    // max_length_to_be_added
    if(len>0){
        cur_len = len;
        if(pair->p_c!=NULL){
            output = bufferevent_get_output(pair->p_c);
        }
        if(output!=NULL){
            evbuffer_add_buffer(output,input);
        }else{
            evbuffer_drain(input,len);
        }
    }
    PROXY_LEAVE(proxy);
    return;
}

// how to handle this? what if in a certain replica,whether we should replica
// this request to all the other replicas?

static void server_side_on_err(struct bufferevent* bev,short what,void* arg){
    socket_pair* pair = arg;
    proxy_node* proxy = pair->proxy;
    PROXY_ENTER(proxy);
    struct timeval recv_time;
    if(what & BEV_EVENT_CONNECTED){
        SYS_LOG(proxy,"Connection Has Established Between %lu And The Real Server.\n",pair->key);
    }else if((what & BEV_EVENT_EOF) || ( what & BEV_EVENT_ERROR)){
        gettimeofday(&recv_time,NULL);
        req_sub_msg* close_msg = build_req_sub_msg(pair->key,pair->counter++,P_CLOSE,0,NULL);
        ((proxy_close_msg*)close_msg->data)->header.received_time = recv_time;
        if(NULL!=close_msg && NULL!=proxy->con_conn){
            bufferevent_write(proxy->con_conn,close_msg,REQ_SUB_SIZE(close_msg));
	// tom add 20150128
	struct evbuffer *output = bufferevent_get_output(proxy->con_conn);
	size_t len = evbuffer_get_length(output);
	printf("Warning: output evbuffer has %lu bytes left when P_CLOSE\n", (unsigned long)len);
	// end tom add
            free(close_msg);
        }
    }
    PROXY_LEAVE(proxy);
    return;
}

static void client_process_data(socket_pair* pair,struct bufferevent* bev,size_t data_size){
    proxy_node* proxy = pair->proxy;
    PROXY_ENTER(proxy)
    void* msg_buf = (char*)malloc(CLIENT_MSG_HEADER_SIZE+data_size);
    req_sub_msg* con_msg=NULL;
    if(NULL==msg_buf){
        goto client_process_data_exit;
    }
    struct evbuffer* evb = bufferevent_get_input(bev);
    evbuffer_remove(evb,msg_buf,CLIENT_MSG_HEADER_SIZE+data_size);
    client_msg_header* msg_header = msg_buf;
    struct timeval recv_time;
    switch(msg_header->type){
        case C_SEND_WR:
            gettimeofday(&recv_time,NULL);
            SYS_LOG(proxy,"Received Msg From Client, The Message Is %s.\n",((client_msg*)msg_header)->data);
            con_msg = build_req_sub_msg(pair->key,pair->counter++,P_SEND,
                    msg_header->data_size,((client_msg*)msg_header)->data);
            ((proxy_send_msg*)con_msg->data)->header.received_time = recv_time;
            if(NULL!=con_msg && NULL!=proxy->con_conn){
                bufferevent_write(proxy->con_conn,con_msg,REQ_SUB_SIZE(con_msg));
		// tom add 20150128
		struct evbuffer *output = bufferevent_get_output(proxy->con_conn);
		size_t len = evbuffer_get_length(output);
		printf("Warning: output evbuffer has %lu bytes left when P_SEND\n", (unsigned long)len);
		// end tom add
            }
            break;
        default:
            SYS_LOG(proxy,"Unknown Client Msg Type %d\n",
                    msg_header->type);
            goto client_process_data_exit;
    }
client_process_data_exit:
    if(NULL!=msg_buf){free(msg_buf);}
    if(NULL!=con_msg){free(con_msg);}
    PROXY_LEAVE(proxy)
    return;
};

// check whether there is enough data on the client evbuffer
static void client_side_on_read(struct bufferevent* bev,void* arg){
    socket_pair* pair = arg;
    proxy_node* proxy = pair->proxy;
    PROXY_ENTER(proxy);
    client_msg_header* header = NULL;
    struct evbuffer* input = bufferevent_get_input(bev);
    size_t len = 0;
    len = evbuffer_get_length(input);
    SYS_LOG(proxy,"There Is %u Bytes Data In The Buffer In Total.\n",
            (unsigned)len);
    while(len>=CLIENT_MSG_HEADER_SIZE){
        header = (client_msg_header*)malloc(CLIENT_MSG_HEADER_SIZE);
        if(NULL==header){return;}
        evbuffer_copyout(input,header,CLIENT_MSG_HEADER_SIZE);
        int data_size = header->data_size;
        if(len>=(CLIENT_MSG_HEADER_SIZE+data_size)){
           client_process_data(pair,bev,data_size); 
        }else{
            break;
        }
        free(header);
        header=NULL;
        len = evbuffer_get_length(input);
    }
    if(NULL!=header){free(header);}
    PROXY_LEAVE(proxy) 
    return;
};

static void client_side_on_err(struct bufferevent* bev,short what,void* arg){
    socket_pair* pair = arg;
    proxy_node* proxy = pair->proxy;
    PROXY_ENTER(proxy);
    struct timeval recv_time;
    if(what&BEV_EVENT_CONNECTED){
        SYS_LOG(proxy,"Client %lu Connects.\n",pair->key);

    }else if((what & BEV_EVENT_EOF) || ( what & BEV_EVENT_ERROR)){
        gettimeofday(&recv_time,NULL);
        req_sub_msg* close_msg = build_req_sub_msg(pair->key,pair->counter++,P_CLOSE,0,NULL);
        ((proxy_close_msg*)close_msg->data)->header.received_time = recv_time;
        if(NULL!=close_msg && NULL!=proxy->con_conn){
            bufferevent_write(proxy->con_conn,close_msg,REQ_SUB_SIZE(close_msg));
            free(close_msg);
        }
    }
    PROXY_LEAVE(proxy);
    return;
}


static req_sub_msg* build_req_sub_msg(hk_t s_key,counter_t counter,int type,size_t data_size,void* data){
    req_sub_msg* msg = NULL;
    switch(type){
        case P_CONNECT:
            msg = (req_sub_msg*)malloc(SYS_MSG_HEADER_SIZE+PROXY_CONNECT_MSG_SIZE);
            msg->header.type = REQUEST_SUBMIT;
            msg->header.data_size = PROXY_CONNECT_MSG_SIZE;
            proxy_connect_msg* co_msg = (void*)msg->data;
            co_msg->header.action = P_CONNECT;
            co_msg->header.connection_id = s_key;
            co_msg->header.counter = counter;
            gettimeofday(&co_msg->header.created_time,NULL);
            break;
        case P_SEND:
            msg = (req_sub_msg*)malloc(SYS_MSG_HEADER_SIZE+sizeof(proxy_send_msg)+data_size);
            msg->header.type = REQUEST_SUBMIT;
            msg->header.data_size = sizeof(proxy_send_msg)+data_size;
            proxy_send_msg* send_msg = (void*)msg->data;
            send_msg->header.action = P_SEND;
            send_msg->header.connection_id = s_key;
            send_msg->header.counter = counter;
            send_msg->data_size = data_size;
            memcpy(send_msg->data,data,data_size);
            gettimeofday(&send_msg->header.created_time,NULL);
            break;
        case P_CLOSE:
            msg = (req_sub_msg*)malloc(SYS_MSG_HEADER_SIZE+PROXY_CONNECT_MSG_SIZE);
            msg->header.type = REQUEST_SUBMIT;
            msg->header.data_size = PROXY_CONNECT_MSG_SIZE;
            proxy_close_msg* cl_msg = (void*)msg->data;
            cl_msg->header.action = P_CLOSE;
            cl_msg->header.connection_id = s_key;
            cl_msg->header.counter = counter;
            gettimeofday(&cl_msg->header.created_time,NULL);
            break;
        default:
            goto build_req_sub_msg_err_exit;
    }
    return msg;
build_req_sub_msg_err_exit:
    if(NULL!=msg){
        free(msg);
    }
    return NULL;
}

static void proxy_on_accept(struct evconnlistener* listener,evutil_socket_t
        fd,struct sockaddr *address,int socklen,void *arg){
    req_sub_msg* req_msg = NULL;
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    SYS_LOG(proxy,"In Proxy : A Client Connection Is Established : %d.\n",fd);
    if(proxy->con_conn==NULL){
        SYS_LOG(proxy,"In Proxy : We Have Lost The Connection To Consensus Component Now.\n");
        close(fd);
    }else{
        socket_pair* new_conn = malloc(sizeof(socket_pair));
        memset(new_conn,0,sizeof(socket_pair));
        struct timeval cur;
        gettimeofday(&cur,NULL);
        new_conn->key = gen_key(proxy->node_id,proxy->pair_count++,
                cur.tv_sec);
        new_conn->p_c = bufferevent_socket_new(proxy->base,fd,BEV_OPT_CLOSE_ON_FREE);
        new_conn->counter = 0;
        new_conn->proxy = proxy;
        bufferevent_setcb(new_conn->p_c,client_side_on_read,NULL,client_side_on_err,new_conn);
        bufferevent_enable(new_conn->p_c,EV_READ|EV_PERSIST|EV_WRITE);
        MY_HASH_SET(new_conn,proxy->hash_map);
        // connect operation should be consistent among all the proxies.
        struct timeval recv_time;
        gettimeofday(&recv_time,NULL);
        req_msg = build_req_sub_msg(new_conn->key,new_conn->counter++,P_CONNECT,0,NULL); 
        ((proxy_connect_msg*)req_msg->data)->header.received_time = recv_time;
        bufferevent_write(proxy->con_conn,req_msg,REQ_SUB_SIZE(req_msg));
	// tom add 20150128
	struct evbuffer *output = bufferevent_get_output(proxy->con_conn);
	size_t len = evbuffer_get_length(output);
	printf("Warning: output evbuffer has %lu bytes left when P_CONNECT\n", (unsigned long)len);
	// end tom add
    }
    if(req_msg!=NULL){
        free(req_msg);
    }
    PROXY_LEAVE(proxy);
    return;
}
/*
static void proxy_singnal_handler_sys(int sig){
    
    proxy_node* proxy = hack_arg;
    if(sig&SIGTERM){
        SYS_LOG(proxy,"Node Proxy Received SIGTERM .Now Quit.\n");
        if(proxy->sub_thread!=0){
            pthread_kill(proxy->sub_thread,SIGUSR1);
            SYS_LOG(proxy,"Wating Consensus Comp To Quit.\n");
            pthread_join(proxy->sub_thread,NULL);
        }
    }
    event_base_loopexit(proxy->base,NULL);
    
    return;
}
*/
static void proxy_singnal_handler(evutil_socket_t fid,short what,void* arg){
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    int sig;
    sig = event_get_signal((proxy->sig_handler));
    SYS_LOG(proxy,"PROXY : GET SIGUSR2,Now Check Pending Requests.\n");
    real_do_action(proxy);
    PROXY_LEAVE(proxy);
    return;
}

static void proxy_signal_term(evutil_socket_t fid,short what,void* arg){
    proxy_node* proxy = arg;
    PROXY_ENTER(proxy);
    int sig;
    sig = event_get_signal((proxy->sig_handler));
    SYS_LOG(proxy,"PROXY : GET SIGTERM.\n");
    if(proxy->sub_thread!=0){
        pthread_kill(proxy->sub_thread,SIGUSR1);
        SYS_LOG(proxy,"PROXY : Waiting Consensus Comp To Quit.\n");
        pthread_join(proxy->sub_thread,NULL);
    }
    event_base_loopexit(proxy->base,NULL);
    PROXY_LEAVE(proxy);
    return;
}

proxy_node* proxy_init(int node_id,const char* start_mode,const char* config_path,
        const char* log_path,int fake_mode){
    
    // tom add 20150127
    cpu_set_t cpuset_0, cpuset_1;
    int ret, cpu_nums, core_0, core_1;
    cpu_nums = sysconf(_SC_NPROCESSORS_CONF);/*get the cpu nums*/  
    assert(cpu_nums > 0);  
    // end tom add

    proxy_node* proxy = (proxy_node*)malloc(sizeof(proxy_node));

    if(NULL==proxy){
        err_log("PROXY : Cannot Malloc Memory For The Proxy.\n");
        goto proxy_exit_error;
    }

    memset(proxy,0,sizeof(proxy_node));

    proxy->node_id = node_id;
    proxy->fake = fake_mode;
    proxy->action_period.tv_sec = 0;
    proxy->action_period.tv_usec = 1000000;
    proxy->recon_period.tv_sec = 2;
    proxy->recon_period.tv_usec = 0;
    proxy->p_self = pthread_self();
    // tom add 20150127
     core_0 = 0 % cpu_nums;  
     CPU_ZERO(&cpuset_0);  
     CPU_SET(core_0, &cpuset_0);  
     ret = pthread_setaffinity_np(proxy->p_self, sizeof(cpu_set_t), &cpuset_0);  
     assert(ret == 0);  
     /* Check the actual affinity mask assigned to the thread */  
     ret = pthread_getaffinity_np(proxy->p_self , sizeof(cpu_set_t), &cpuset_0);  
     assert(ret == 0);  
    // end tom add
    if(proxy_read_config(proxy,config_path)){
        err_log("PROXY : Configuration File Reading Error.\n");
        goto proxy_exit_error;
    }

    if(pthread_mutex_init(&proxy->lock,NULL)){
        err_log("PROXY : Cannot Init The Lock.\n");
        goto proxy_exit_error;
    }
    
    // set up base
	struct event_base* base = event_base_new();

    if(NULL==base){
        goto proxy_exit_error;
    }

//    proxy->log_file = NULL; 

    int build_log_ret = 0;
    if(log_path==NULL){
        log_path = ".";
    }else{
        if((build_log_ret=mkdir(log_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))!=0){
            if(errno!=EEXIST){
                err_log("PROXY : Log Directory Creation Failed,No Log Will Be Recorded.\n");
            }else{
                build_log_ret = 0;
            }
        }
    }

    if(!build_log_ret){
        //if(proxy->sys_log || proxy->stat_log){
            char* sys_log_path = (char*)malloc(sizeof(char)*strlen(log_path)+50);
            memset(sys_log_path,0,sizeof(char)*strlen(log_path)+50);
            //err_log("%s.\n",log_path);
            if(NULL!=sys_log_path){
                sprintf(sys_log_path,"%s/node-%u-proxy-sys.log",log_path,proxy->node_id);
                //err_log("%s.\n",sys_log_path);
                proxy->sys_log_file = fopen(sys_log_path,"w");
                free(sys_log_path);
            }
            if(NULL==proxy->sys_log_file && (proxy->sys_log || proxy->stat_log)){
                err_log("PROXY : System Log File Cannot Be Created.\n");
            }
        //}
        //if(proxy->req_log){
            char* req_log_path = (char*)malloc(sizeof(char)*strlen(log_path)+50);
            memset(req_log_path,0,sizeof(char)*strlen(log_path)+50);
            if(NULL!=req_log_path){
                sprintf(req_log_path,"%s/node-%u-proxy-req.log",log_path,proxy->node_id);
                //err_log("%s.\n",req_log_path);
                proxy->req_log_file = fopen(req_log_path,"w");
                free(req_log_path);
            }
            if(NULL==proxy->req_log_file && proxy->req_log){
                err_log("PROXY : Client Request Log File Cannot Be Created.\n");
            }
        //}
    }
    proxy->base = base;
    //proxy->do_action = evtimer_new(proxy->base,proxy_do_action,(void*)proxy);
    // Open database 
    proxy->db_ptr = initialize_db(proxy->db_name,0);

    // if we cannot create the database and the proxy runs not in the fake mode, then we will fail 
    if(proxy->db_ptr==NULL && !proxy->fake){
        err_log("PROXY : Cannot Set Up The Database.\n");
        goto proxy_exit_error;
    }
    
    // ensure the value is NULL at first
    proxy->hash_map=NULL;
    proxy->listener = evconnlistener_new_bind(proxy->base,proxy_on_accept,
                (void*)proxy,LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
                -1,(struct sockaddr*)&proxy->sys_addr.p_addr,
                sizeof(proxy->sys_addr.p_addr));

    if(proxy->listener==NULL){
        err_log("PROXY : Cannot Set Up The IP Address Listener.\n");
        goto proxy_exit_error;
    }

    if(proxy->fake){ 
        proxy->con_node = system_initialize(node_id,start_mode,
                config_path,log_path,1,fake_update_state,proxy->db_ptr,proxy);
    }else{
        proxy->con_node = system_initialize(node_id,start_mode,
                config_path,log_path,0,update_state,NULL,proxy);
    }

    if(NULL==proxy->con_node){
        err_log("PROXY : Cannot Initialize Consensus Component.\n");
        goto proxy_exit_error;
    }

    //register signal handler
    //signal(SIGTERM,proxy_singnal_handler_sys);
    proxy->sig_handler = evsignal_new(proxy->base,
            SIGUSR2,proxy_singnal_handler,proxy);
    evsignal_add(proxy->sig_handler,NULL);

    struct event* sig_term = evsignal_new(proxy->base,
            SIGTERM,proxy_signal_term,proxy);
    evsignal_add(sig_term,NULL);

    pthread_create(&proxy->sub_thread,NULL,t_consensus,proxy->con_node);
    // tom add 20150127
     core_1 = 1 % cpu_nums;  
     CPU_ZERO(&cpuset_1);  
     CPU_SET(core_1, &cpuset_1);  
     ret = pthread_setaffinity_np(proxy->sub_thread, sizeof(cpu_set_t), &cpuset_1);  
     assert(ret == 0);  
     /* Check the actual affinity mask assigned to the thread */  
     ret = pthread_getaffinity_np(proxy->sub_thread , sizeof(cpu_set_t), &cpuset_1);  
     assert(ret == 0);  
    // end tom add
    hack_arg = proxy;

	return proxy;

proxy_exit_error:
    if(NULL!=proxy){
        if(NULL!=proxy->con_node){
            // to do
            if(proxy->sub_thread!=0){
                pthread_kill(proxy->sub_thread,SIGQUIT);
                err_log("PROXY : Wating Consensus Comp To Quit.\n");
                pthread_join(proxy->sub_thread,NULL);
            }
        }
        free(proxy);
    }
    return NULL;
}

void proxy_run(proxy_node* proxy){
    proxy->re_con = evtimer_new(proxy->base,
            reconnect_on_timeout,proxy);
    connect_consensus(proxy);
    if(proxy->do_action!=NULL){
        evtimer_add(proxy->do_action,&proxy->action_period);
    }
    event_base_dispatch(proxy->base);
    proxy_exit(proxy);
    return;
}

void proxy_exit(proxy_node* proxy){
    //to-do
    return;
}

