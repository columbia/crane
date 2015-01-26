#include "../include/util/common-header.h"
#include "../include/replica-sys/node.h"
#include "../include/config-comp/config-comp.h"
#include <sys/stat.h>

int max_waiting_connections = MAX_ACCEPT_CONNECTIONS; 
static unsigned current_connection = 3;

static void usage(){
    err_log("Usage : -n NODE_ID\n");
    err_log("        -m [sr] Start Mode seed|recovery\n");
    err_log("        -c path path to configuration file\n");
}

//msg handler
static void replica_on_read(struct bufferevent*,void*);
static void replica_on_error_cb(struct bufferevent*,short,void*);

//signal handler
static void node_singal_handler(evutil_socket_t fid,short what,void* arg);
static void node_sys_sig_handler(int sig);

// consensus part
static void send_for_consensus_comp(node*,size_t,void*,int);


//concrete message
static void handle_ping_ack(node* ,ping_ack_msg* );
static void handle_ping_req(node* ,ping_req_msg* );


//listener
static void replica_on_accept(struct evconnlistener* listener,evutil_socket_t
        fd,struct sockaddr *address,int socklen,void *arg);

//peer connection
static void peer_node_on_event(struct bufferevent* bev,short ev,void* arg);
static void peer_node_on_read(struct bufferevent*,void*);
static void connect_peer(peer*);
static void peer_node_on_timeout(int fd,short what,void* arg);
static void connect_peers(node*);


//ping part
static void leader_ping_period(int fd,short what,void* arg);
static void expected_leader_ping_period(int fd,short wat,void* arg);
static int initialize_leader_ping(node*);
static int initialize_expect_ping(node*);

//make progress
static int initialize_leader_make_progress(node* my_node);
static void make_progress_on(int fd,short what,void* arg);

//view change
static void update_view(node*,view*);
static void become_leader(node*);
static void giveup_leader(node*);
static void replica_sync(node*);

//free related function
static void free_peers(node*);
static int free_node(node*);

//helper function
static int isLeader(node*);

//implementation level

static void node_singal_handler(evutil_socket_t fid,short what,void* arg){
    node* my_node = arg;
    if(what&EV_SIGNAL){
        SYS_LOG(my_node,"Node %d Received Kill Singal.Now Quit.\n",my_node->node_id);
    }
    event_base_loopexit(my_node->base,NULL);
}

static void peer_node_on_read(struct bufferevent* bev,void* arg){return;};

static void peer_node_on_event(struct bufferevent* bev,short ev,void* arg){
    peer* peer_node = (peer*)arg;
    node* my_node = peer_node->my_node;
    if(ev&BEV_EVENT_CONNECTED){
        SYS_LOG(my_node,"Connected to Node %d\n",peer_node->peer_id);
        peer_node->active = 1;
    }else if((ev & BEV_EVENT_EOF )||(ev&BEV_EVENT_ERROR)){
        if(peer_node->active){
            peer_node->active = 0;
            SYS_LOG(my_node,"Lost Connection With Node %d \n",peer_node->peer_id);
        }
        peer_node->my_buff_event = NULL;
        int err = EVUTIL_SOCKET_ERROR();
		SYS_LOG(my_node,"%s (%d)\n",evutil_socket_error_to_string(err),peer_node->peer_id);
        bufferevent_free(bev);
        event_add(peer_node->reconnect,&my_node->config.reconnect_timeval);
    }
};


static void connect_peer(peer* peer_node){
    peer_node->my_buff_event = bufferevent_socket_new(peer_node->base,-1,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(peer_node->my_buff_event,peer_node_on_read,NULL,peer_node_on_event,peer_node);
    bufferevent_enable(peer_node->my_buff_event,EV_READ|EV_WRITE|EV_PERSIST);
    bufferevent_socket_connect(peer_node->my_buff_event,(struct sockaddr*)peer_node->peer_address,peer_node->sock_len);
};

static void peer_node_on_timeout(int fd,short what,void* arg){
    connect_peer((peer*)arg);
};


static void connect_peers(node* my_node){
    for(uint32_t i=0;i<my_node->group_size;i++){
        if(i!=my_node->node_id){
            peer* peer_node = my_node->peer_pool+i;
            peer_node->reconnect = evtimer_new(peer_node->base,peer_node_on_timeout,peer_node);
            connect_peer(peer_node);
        }
    }
}


static void lost_connection_with_leader(node* my_node){
    SYS_LOG(my_node,"Node %u Lost Connection With The Leader\n",
            my_node->node_id);
    SYS_LOG(my_node,"Node %u Will Start A Leader Election\n",
            my_node->node_id);
    return;
}

static void expected_leader_ping_period(int fd,short what,void* arg){
    node* my_node = arg;
    if(my_node->node_id==my_node->cur_view.leader_id){
        if(NULL!=my_node->ev_leader_ping){
            event_free(my_node->ev_leader_ping);
            initialize_leader_ping(my_node);
        }
    }else{
        struct timeval* last = &my_node->last_ping_msg;
        struct timeval cur;
        gettimeofday(&cur,NULL);
        struct timeval temp;
        timeval_add(last,&my_node->config.expect_ping_timeval,&temp);
        if(timeval_comp(&temp,&cur)>=0){
            event_add(my_node->ev_leader_ping,&my_node->config.expect_ping_timeval);
        }else{
            SYS_LOG(my_node,
                    "Node %d Haven't Heard From The Leader\n",
                    my_node->node_id);
            lost_connection_with_leader(my_node);
        }
    }
}


static void leader_ping_period(int fd,short what,void* arg){
    node* my_node = arg; 
    SYS_LOG(my_node,"Leader Tries To Ping Other Nodes\n");
    // at first check whether I am the leader
    if(my_node->cur_view.leader_id!=my_node->node_id){
        if(my_node->ev_leader_ping!=NULL){
            event_free(my_node->ev_leader_ping);
            initialize_expect_ping(my_node);
        }
        return;
    }else{
        void* ping_req = build_ping_req(my_node->node_id,&my_node->cur_view);
        if(NULL==ping_req){
            goto add_ping_event;
        }
        for(uint32_t i=0;i<my_node->group_size;i++){
            if(i!=my_node->node_id && my_node->peer_pool[i].active){
                struct bufferevent* buff = my_node->peer_pool[i].my_buff_event;
                bufferevent_write(buff,ping_req,PING_REQ_SIZE);
                SYS_LOG(my_node,
                        "Send Ping Msg To Node %u\n",i);
            }
        }
        if(NULL!=ping_req){
            free(ping_req);
        }
    add_ping_event:
        if(NULL!=my_node->ev_leader_ping){
            event_add(my_node->ev_leader_ping,&my_node->config.ping_timeval);
        }
    }
};

static int initialize_leader_ping(node* my_node){
    if(NULL==my_node->ev_leader_ping){
        my_node->ev_leader_ping = evtimer_new(my_node->base,leader_ping_period,(void*)my_node);
        if(my_node->ev_leader_ping==NULL){
            return 1;
        }
    }
    event_add(my_node->ev_leader_ping,&my_node->config.ping_timeval);
    return 0;
}

static int initialize_expect_ping(node* my_node){
    if(NULL==my_node->ev_leader_ping){
        my_node->ev_leader_ping = evtimer_new(my_node->base,expected_leader_ping_period,(void*)my_node);
        if(my_node->ev_leader_ping==NULL){
            return 1;
        }
    }
    event_add(my_node->ev_leader_ping,&my_node->config.expect_ping_timeval);
    return 0;
}

static void make_progress_on(int fd,short what,void* arg){
    node* my_node = arg; 
    SYS_LOG(my_node,"Leader Tries To Make Progress.\n");
    // at first check whether I am the leader
    if(my_node->cur_view.leader_id!=my_node->node_id){
        if(my_node->ev_make_progress!=NULL){
            event_free(my_node->ev_make_progress);
            initialize_leader_make_progress(my_node);
        }
        return;
    }
    if(NULL!=my_node->consensus_comp){
        consensus_make_progress(my_node->consensus_comp);
    }
    if(NULL!=my_node->ev_make_progress){
        event_add(my_node->ev_make_progress,&my_node->config.make_progress_timeval);
    }
    return;
}

static int initialize_leader_make_progress(node* my_node){
    if(NULL==my_node->ev_make_progress){
        my_node->ev_make_progress = evtimer_new(my_node->base,make_progress_on,(void*)my_node);
        if(my_node->ev_make_progress==NULL){
            return 1;
        }
    }
    event_add(my_node->ev_make_progress,&my_node->config.make_progress_timeval);
    return 0;
}



static void update_view(node* my_node,view* new_view){
    SYS_LOG(my_node,"Node %d Entered Update View\n",
            my_node->node_id);
    int old_leader = isLeader(my_node);
    memcpy(&my_node->cur_view,new_view,sizeof(view));
    int new_leader = isLeader(my_node);
    if(old_leader!=new_leader){
        if(new_leader){
            become_leader(my_node);
        }else{
            giveup_leader(my_node);
        }
    }
    SYS_LOG(my_node,"Node %d 's Current View Changed To %u \n",
        my_node->node_id,my_node->cur_view.view_id);
    return;
}

static void become_leader(node* my_node){
    return;
}
static void giveup_leader(node* my_node){
}
static void replica_sync(node* my_node){
    return;
}

static void free_peer(peer* peer_node){
    if(NULL!=peer_node){
        if(NULL!=peer_node->my_buff_event){
            bufferevent_free(peer_node->my_buff_event);
        }
        if(NULL!=peer_node->reconnect){
            event_free(peer_node->reconnect);
        }
        if(NULL!=peer_node->peer_address){
            free(peer_node->peer_address);
        }
        free(peer_node);
    }
}

static void free_peers(node* my_node){
    for(uint32_t i=0;i<my_node->group_size;i++){
        free_peer(my_node->peer_pool+i);
    }
}

static int isLeader(node* my_node){
    return my_node->cur_view.leader_id==my_node->node_id;
}

static int free_node(node* my_node){
    
    if(my_node->listener!=NULL){
        evconnlistener_free(my_node->listener);
    }
    if(my_node->signal_handler!=NULL){
        event_free(my_node->signal_handler);
    }
    if(my_node->ev_make_progress!=NULL){
        event_free(my_node->ev_make_progress);
    }
    if(my_node->ev_leader_ping!=NULL){
        event_free(my_node->ev_leader_ping);
    }
    if(my_node->base!=NULL){
        event_base_free(my_node->base);
    }
    
    return 0;
}


static void replica_on_error_cb(struct bufferevent* bev,short ev,void *arg){
    if(ev&BEV_EVENT_EOF){
        // the connection has been closed;
        // do the cleaning
    }
    return;
}

static void replica_on_accept(struct evconnlistener* listener,evutil_socket_t fd,struct sockaddr *address,int socklen,void *arg){
    node* my_node = arg;
    SYS_LOG(my_node, "A New Connection Is Established.\n");
    struct bufferevent* new_buff_event = bufferevent_socket_new(my_node->base,fd,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(new_buff_event,replica_on_read,NULL,replica_on_error_cb,(void*)my_node);
    bufferevent_enable(new_buff_event,EV_READ|EV_PERSIST|EV_WRITE);
};

// consensus part
static void send_for_consensus_comp(node* my_node,size_t data_size,void* data,int target){
    
    consensus_msg* msg = build_consensus_msg(data_size,data);
    if(NULL==msg){
        goto send_for_consensus_comp_exit;
    }
    // means send to every node except me
    if(target<0){
        for(uint32_t i=0;i<my_node->group_size;i++){
            if(i!=my_node->node_id && my_node->peer_pool[i].active){
                struct bufferevent* buff = my_node->peer_pool[i].my_buff_event;
                bufferevent_write(buff,msg,CONSENSUS_MSG_SIZE(msg));
                SYS_LOG(my_node,
                        "Send Consensus Msg To Node %u\n",i);
            }
        }
    }else{
        if(target!=(int)my_node->node_id&&my_node->peer_pool[target].active){
            struct bufferevent* buff = my_node->peer_pool[target].my_buff_event;
            bufferevent_write(buff,msg,CONSENSUS_MSG_SIZE(msg));
            SYS_LOG(my_node,
                    "Send Consensus Msg To Node %u.\n",target);
        }
    }
send_for_consensus_comp_exit:
    if(msg!=NULL){
        free(msg);
    }
    
    return;
}

static void handle_ping_ack(node* my_node,ping_ack_msg* msg){
    if(my_node->cur_view.view_id < msg->view.view_id){
        update_view(my_node,&msg->view);
    }else{
        SYS_LOG(my_node,
                "Ignore Ping Ack From Node %u.\n",msg->node_id);
    }
}

static void handle_ping_req(node* my_node,ping_req_msg* msg){
    SYS_LOG(my_node,
            "Received Ping Req Msg In Node %u From Node %u.",
        my_node->node_id,msg->node_id);
    if(my_node->cur_view.view_id < msg->view.view_id){
        update_view(my_node,&msg->view);
    }else if(my_node->cur_view.view_id > msg->view.view_id){
        if(my_node->peer_pool[msg->node_id].active){
            void* ping_ack = build_ping_ack(my_node->node_id,&my_node->cur_view);
            if(NULL!=ping_ack){
                struct bufferevent* buff = my_node->peer_pool[msg->node_id].my_buff_event;
                bufferevent_write(buff,ping_ack,PING_REQ_SIZE);
                SYS_LOG(my_node,
                    "Send Ping Ack To Lagged Node %u.\n",msg->node_id);
                free(ping_ack);
            }
        } 
        return;
    }
    if(my_node->cur_view.view_id == msg->view.view_id){
        if(!isLeader(my_node)){
            if(timeval_comp(&my_node->last_ping_msg,&msg->timestamp)<0){
                memcpy(&my_node->last_ping_msg,&msg->timestamp,
                        sizeof(struct timeval));
            }
            if(NULL!=my_node->ev_leader_ping){
                evtimer_del(my_node->ev_leader_ping);
                evtimer_add(my_node->ev_leader_ping,&my_node->config.expect_ping_timeval);
            }
        }else{
            // leader should not receive the ping req, otherwise the sender is
            // lagged behind,otherwise the leader is outdated which will be
            // treated as a smaller cur view than what in the msg but when they have 
            // the same view id, which can be ignored
            SYS_LOG(my_node,"Received Ping Req From %u In View %u\n",
                    msg->node_id,msg->view.view_id);
        }
    }
    return;
}

static void handle_consensus_msg(node* my_node,consensus_msg* msg){
    SYS_LOG(my_node,"Node %d Received Consensus Message\n",
            my_node->node_id);
    if(NULL!=my_node->consensus_comp){
        consensus_handle_msg(my_node->consensus_comp,msg->header.data_size,(void*)msg+SYS_MSG_HEADER_SIZE);
    }
    return;
}

static void handle_request_submit(node* my_node,
        req_sub_msg* msg,struct bufferevent* evb){
    SYS_LOG(my_node,"Node %d Received Consensus Submit Request\n",
            my_node->node_id);
    SYS_LOG(my_node,"The Data Size Is %lu \n",
            msg->header.data_size);
    if(NULL!=my_node->consensus_comp){
        view_stamp return_vs;
        consensus_submit_request(
                my_node->consensus_comp,msg->header.data_size,
                (void*)msg+SYS_MSG_HEADER_SIZE,&return_vs);
    }
    return;
}

static void handle_msg(node* my_node,struct bufferevent* bev,size_t data_size){
    //debug_log("there is enough data to read,actual data handler is called\n");
    void* msg_buf = (char*)malloc(SYS_MSG_HEADER_SIZE+data_size);
    if(NULL==msg_buf){
        goto handle_msg_exit;
    }
    struct evbuffer* evb = bufferevent_get_input(bev);
    evbuffer_remove(evb,msg_buf,SYS_MSG_HEADER_SIZE+data_size);
    sys_msg_header* msg_header = msg_buf;
    switch(msg_header->type){
        case PING_ACK:
            handle_ping_ack(my_node,(ping_ack_msg*)msg_buf);
            break;
        case PING_REQ:
            handle_ping_req(my_node,(ping_req_msg*)msg_buf);
            break;
        case CONSENSUS_MSG:
            handle_consensus_msg(my_node,(consensus_msg*)msg_buf);
            break;
        case REQUEST_SUBMIT:
            handle_request_submit(my_node,(req_sub_msg*)msg_buf,bev);
            break;
        default:
            SYS_LOG(my_node,"Unknown Msg Type %d\n",
                    msg_header->type);
            goto handle_msg_exit;
    }

handle_msg_exit:
    if(NULL!=msg_buf){free(msg_buf);}
    return;
}

//general data handler by the user, test if there is enough data to read
static void replica_on_read(struct bufferevent* bev,void* arg){
    node* my_node = arg;
    sys_msg_header* buf = NULL;;
    struct evbuffer* input = bufferevent_get_input(bev);
    size_t len = 0;
    len = evbuffer_get_length(input);
    SYS_LOG(my_node,"Enter Consensus Communication Module.\n");
    int counter = 0;
    SYS_LOG(my_node,"There Is %u Bytes Data In The Buffer In Total.\n",
            (unsigned)len);
    while(len>=SYS_MSG_HEADER_SIZE){
        buf = (sys_msg_header*)malloc(SYS_MSG_HEADER_SIZE);
        if(NULL==buf){return;}
        evbuffer_copyout(input,buf,SYS_MSG_HEADER_SIZE);
        int data_size = buf->data_size;
        if(len>=(SYS_MSG_HEADER_SIZE+data_size)){
           my_node->msg_cb(my_node,bev,data_size); 
           counter++;
        }else{
            break;
        }
        free(buf);
        buf=NULL;
        len = evbuffer_get_length(input);
    }
    if(my_node->stat_log){
        STAT_LOG(my_node,"This Function Call Process %u Requests In Total.\n",counter);
    }
    if(NULL!=buf){free(buf);}
    return;
}


int initialize_node(node* my_node,const char* log_path,int deliver_mode,void (*user_cb)(size_t data_size,void* data,void* arg),void* db_ptr,void* arg){
    
    int flag = 1;
    gettimeofday(&my_node->last_ping_msg,NULL);
    if(my_node->cur_view.leader_id==my_node->node_id){
        if(initialize_leader_ping(my_node)){
            goto initialize_node_exit;
        }
        if(initialize_leader_make_progress(my_node)){
            goto initialize_node_exit;
        }
    }
    else{
        if(initialize_expect_ping(my_node)){
            goto initialize_node_exit;
        }
    }

    int build_log_ret = 0;
    if(log_path==NULL){
        log_path = ".";
    }else{
        if((build_log_ret=mkdir(log_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))!=0){
            if(errno!=EEXIST){
                err_log("CONSENSUS MODULE : Log Directory Creation Failed,No Log Will Be Recorded.\n");
            }else{
                build_log_ret = 0;
            }
        }
    }
    if(!build_log_ret){
        if(my_node->sys_log || my_node->stat_log){
            char* sys_log_path = (char*)malloc(sizeof(char)*strlen(log_path)+50);
            memset(sys_log_path,0,sizeof(char)*strlen(log_path)+50);
            if(NULL!=sys_log_path){
                sprintf(sys_log_path,"%s/node-%u-consensus-sys.log",log_path,my_node->node_id);
                my_node->sys_log_file = fopen(sys_log_path,"w");
                free(sys_log_path);
            }
            if(NULL==my_node->sys_log_file){
                err_log("CONSENSUS MODULE : System Log File Cannot Be Created.\n");
            }
        }
    }
     
//    my_node->signal_handler = evsignal_new(my_node->base,
//            SIGQUIT,node_singal_handler,my_node);
//    evsignal_add(my_node->signal_handler,NULL);
    my_node->state = NODE_ACTIVE;
    my_node->msg_cb = handle_msg;
    connect_peers(my_node);
    my_node->consensus_comp = NULL;

    my_node->consensus_comp = init_consensus_comp(my_node,
            my_node->node_id,my_node->sys_log_file,my_node->sys_log,
            my_node->stat_log,my_node->db_name,deliver_mode,db_ptr,my_node->group_size,
            &my_node->cur_view,user_cb,send_for_consensus_comp,arg);
    if(NULL==my_node->consensus_comp){
        goto initialize_node_exit;
    }
    flag = 0;
initialize_node_exit:
        return flag;
}

node* system_initialize(int node_id,const char* start_mode,const char* config_path,const char* log_path,int deliver_mode,void(*user_cb)(int data_size,void* data,void* arg),void* db_ptr,void* arg){

    node* my_node = (node*)malloc(sizeof(node));
    memset(my_node,0,sizeof(node));
    if(NULL==my_node){
        goto exit_error;
    }
    // set up base
	struct event_base* base = event_base_new();
    if(NULL==base){
        goto exit_error;
    }

    my_node->base = base;
    my_node->node_id = node_id;

    //seed, currently the node is the leader
    if(*start_mode=='s'){
        my_node->cur_view.view_id = 1;
        my_node->cur_view.leader_id = my_node->node_id;
        my_node->ev_leader_ping = NULL;
    }else{
        my_node->cur_view.view_id = 0;
        my_node->cur_view.leader_id = 9999;
        my_node->ev_leader_ping = NULL;
    }
    
    my_node->config.make_progress_timeval.tv_sec = 1;
    my_node->config.make_progress_timeval.tv_usec = 100;
    my_node->config.ping_timeval.tv_sec = 2;
    my_node->config.ping_timeval.tv_usec = 0;
    my_node->config.expect_ping_timeval.tv_sec = 8;
    my_node->config.expect_ping_timeval.tv_usec = 0;
    my_node->config.reconnect_timeval.tv_sec = 2;
    my_node->config.reconnect_timeval.tv_usec = 0;


    if(consensus_read_config(my_node,config_path)){
        err_log("CONSENSUS MODULE : Configuration File Reading Failed.\n");
        goto exit_error;
    }


    if(initialize_node(my_node,log_path,deliver_mode,user_cb,db_ptr,arg)){
        err_log("CONSENSUS MODULE : Network Layer Initialization Failed.\n");
        goto exit_error;
    }

    my_node->listener =
        evconnlistener_new_bind(base,replica_on_accept,
                (void*)my_node,LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,-1,
                (struct sockaddr*)&my_node->my_address,sizeof(my_node->my_address));

    if(!my_node->listener){
        err_log("CONSENSUS MODULE : Cannot Set Up The Listener.\n");
        goto exit_error;
    }
	return my_node;

exit_error:
    if(NULL!=my_node){
        free_node(my_node);
    }
    return NULL;
}

void system_run(struct node_t* my_node){
    SYS_LOG(my_node,"Node %u Starts Running\n",
            my_node->node_id);
    event_base_dispatch(my_node->base);
}

void system_exit(struct node_t* my_node){
    event_base_loopexit(my_node->base,NULL);
    free_node(my_node);
}
