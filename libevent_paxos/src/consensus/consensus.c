/*
 * =====================================================================================
 *
 *       Filename:  consensus.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/06/2014 02:13:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  *
 * =====================================================================================
 */

#include "../include/consensus/consensus.h"
#include "../include/consensus/consensus-msg.h"
#include "../include/db/db-interface.h"

typedef struct request_record_t{
    struct timeval created_time; // data created timestamp
    char is_closed;
    uint64_t bit_map; // now we assume the maximal replica group size is 64;
    size_t data_size; // data size
    char data[0];     // real data
}__attribute__((packed))request_record;
#define REQ_RECORD_SIZE(M) (sizeof(request_record)+(M->data_size))

typedef struct view_boundary_t{
    view_stamp last_boundary;
}view_boundary;
#define VIEW_BOUNDARY_SIZE (sizeof(view_boundary))

typedef struct consensus_component_t{ con_role my_role;
    uint32_t node_id;

    int deliver_mode;
    uint32_t group_size;
    struct node_t* my_node;

    FILE* sys_log_file;
    int sys_log;
    int stat_log;

    view* cur_view;
    view_stamp highest_seen_vs; 
    view_stamp highest_to_commit_vs;
    view_stamp highest_committed_vs;

    db* db_ptr;
    up_call uc;
    user_cb ucb;
    void* up_para;
}consensus_component;

typedef uint64_t record_index_type;

//static function declaration
//

static view_stamp get_next_view_stamp(consensus_component*);

static void view_stamp_inc(view_stamp*);
static void cross_view(view_stamp*);

static int leader_handle_submit_req(struct consensus_component_t*,
        size_t,void*,view_stamp*);

static int forward_submit_req(consensus_component*,size_t,void*);

static void handle_accept_req(consensus_component*,void* );
static void handle_accept_ack(consensus_component* ,void* );
static void handle_missing_req(consensus_component*,void* );
static void handle_missing_ack(consensus_component*,void* );
static void handle_force_exec(consensus_component* ,void* );
static void handle_forward_req(consensus_component*,void* );

static void* build_accept_req(consensus_component* ,size_t ,void*, view_stamp*);
static void* build_accept_ack(consensus_component* ,view_stamp*);
static void* build_missing_req(consensus_component*,view_stamp*);
static void* build_missing_ack(consensus_component*,view_stamp*);
static void* build_force_exec(consensus_component*);
static void* build_forward_req(consensus_component* comp,
        size_t,void*);

static void try_to_execute(consensus_component*);
static void leader_try_to_execute(consensus_component*);
static void send_missing_req(consensus_component*,view_stamp*);

// helper func
static int isLeader(consensus_component*);


//static void deliver_msg_vs(consensus_component*,const view_stamp*);
static void deliver_msg_data(consensus_component*,view_stamp*);

void consensus_handle_msg(consensus_component* comp,size_t data_size,void* data){
    consensus_msg_header* header = data;
    size_t trash_data_size = data_size;
    trash_data_size++;
    switch(header->msg_type){
        case ACCEPT_REQ:
            handle_accept_req(comp,data);
            break;
        case ACCEPT_ACK:
            handle_accept_ack(comp,data);
            break;
        case MISSING_REQ:
            handle_missing_req(comp,data);
            break;
        case MISSING_ACK:
            handle_missing_ack(comp,data);
            break;
        case FORCE_EXEC:
            handle_force_exec(comp,data);
            break;
        case FORWARD_REQ:
            handle_forward_req(comp,data);
            break;
        default:
            SYS_LOG(comp,"Unknown Consensus MSG : %d \n",
            header->msg_type);
            break;
    }
    return;
}

//public interface method

view_stamp consensus_get_highest_seen_req(consensus_component* comp){
    return comp->highest_seen_vs;
}


consensus_component* init_consensus_comp(struct node_t* node,uint32_t node_id, FILE* log,
        int sys_log,int stat_log,const char* db_name,
        int deliver_mode,void* db_ptr,int group_size,
        view* cur_view,user_cb u_cb,up_call uc,void* arg){
    
    consensus_component* comp = (consensus_component*)
        malloc(sizeof(consensus_component));
    memset(comp,0,sizeof(consensus_component));

    if(NULL!=comp){
        if(deliver_mode==50){
            comp->db_ptr = db_ptr;
        }else{
            comp->db_ptr = initialize_db(db_name,0);
            if(NULL==comp->db_ptr){
                goto consensus_error_exit;
            }
        }    
        comp->sys_log = sys_log;
        comp->stat_log = stat_log;
        comp->sys_log_file = log;
        comp->my_node = node;
        comp->node_id = node_id;
        comp->deliver_mode = deliver_mode;
        comp->group_size = group_size;
        comp->cur_view = cur_view;
        if(comp->cur_view->leader_id == comp->node_id){
            comp->my_role = LEADER;
        }else{
            comp->my_role = SECONDARY;
        }
        comp->ucb = u_cb;
        comp->uc = uc;
        comp->up_para = arg;
        comp->highest_seen_vs.view_id = 1;
        comp->highest_seen_vs.req_id = 0;
        comp->highest_committed_vs.view_id = 1; 
        comp->highest_committed_vs.req_id = 0; 
        comp->highest_to_commit_vs.view_id = 1;
        comp->highest_to_commit_vs.req_id = 0;
        goto consensus_init_exit;

    }
consensus_error_exit:
    if(comp!=NULL){
        if(NULL!=comp->db_ptr && comp->deliver_mode == 1){
            close_db(comp->db_ptr,0);
        }
        free(comp);
    }

consensus_init_exit:
    return comp;
}

int consensus_submit_request(struct consensus_component_t* comp,
        size_t data_size,void* data,view_stamp* vs){
    if(LEADER==comp->my_role){
       return leader_handle_submit_req(comp,data_size,data,vs);
    }else{
        return forward_submit_req(comp,data_size,data);
    }
}

void consensus_update_role(struct consensus_component_t* comp){
    if(comp->cur_view->leader_id!=comp->node_id){
        comp->my_role = SECONDARY;
    }else{
        comp->my_role = LEADER;
        comp->highest_seen_vs.view_id = comp->cur_view->view_id;
        comp->highest_seen_vs.req_id = 0;
    }
    return;
}

//static method area

static view_stamp get_next_view_stamp(consensus_component* comp){
    view_stamp next_vs;
    next_vs.view_id = comp->highest_seen_vs.view_id;
    next_vs.req_id = (comp->highest_seen_vs.req_id+1);
    return next_vs;
};

static void view_stamp_inc(view_stamp* vs){
    vs->req_id++;
    return;
};

static int isLeader(consensus_component* comp){
    if(comp==NULL){return 0;}
    assert(comp->cur_view!=NULL && "current node has not view");
    return comp->cur_view->leader_id == comp->node_id;
};

static int leader_handle_submit_req(struct consensus_component_t* comp,
        size_t data_size,void* data,view_stamp* vs){
    
    int ret = 1;
    view_stamp next = get_next_view_stamp(comp);
    if(NULL!=vs){
        vs->view_id = next.view_id;
        vs->req_id = next.req_id;
    }
    db_key_type record_no = vstol(&next);
    request_record* record_data = 
        (request_record*)malloc(data_size+sizeof(request_record));
    gettimeofday(&record_data->created_time,NULL);
    record_data->bit_map = (1<<comp->node_id);
    record_data->data_size = data_size;
    record_data->is_closed = 0;
    memcpy(record_data->data,data,data_size);
    if(store_record(comp->db_ptr,sizeof(record_no),&record_no,REQ_RECORD_SIZE(record_data),record_data)){
        goto handle_submit_req_exit;
    }    
    ret = 0;
    view_stamp_inc(&comp->highest_seen_vs);
    if(comp->group_size>1){
        accept_req* msg = build_accept_req(comp,REQ_RECORD_SIZE(record_data),record_data,&next);
        if(NULL==msg){
            goto handle_submit_req_exit;
        }
        comp->uc(comp->my_node,ACCEPT_REQ_SIZE(msg),msg,-1);
        free(msg);
    }else{
        try_to_execute(comp);
    }
handle_submit_req_exit: 
    // no need to care about database, every time we will override it.
    if(record_data!=NULL){
        free(record_data);
    }
    
    return ret;
}

static int forward_submit_req(consensus_component* comp,size_t data_size,void* data){
    
    forward_req* msg = build_forward_req(comp,data_size,data);
    int ret = 1;
    if(msg!=NULL){
        comp->uc(comp->my_node,FORWARD_REQ_SIZE(msg),msg,comp->cur_view->leader_id);
        ret = 0;
    }
    
    return ret;
}

static void update_record(request_record* record,uint32_t node_id){
    record->bit_map = (record->bit_map | (1<<node_id));
    //debug_log("the record bit map is updated to %x\n",record->bit_map);
    return;
}

static int reached_quorum(request_record* record,int group_size){
    // this may be compatibility issue 
    if(__builtin_popcountl(record->bit_map)>=((group_size/2)+1)){
        return 1;
    }else{
        return 0;
    }
}

static void handle_accept_req(consensus_component* comp,void* data){
    SYS_LOG(comp,"Node %d Handle Accept Req.\n",
            comp->node_id);
    accept_req* msg = data;
    if(msg->msg_vs.view_id< comp->cur_view->view_id){
        goto handle_accept_req_exit;
    }
    // if we this message is not from the current leader
    if(msg->msg_vs.view_id == comp->cur_view->view_id && 
            msg->node_id!=comp->cur_view->leader_id){
        goto handle_accept_req_exit;
    }
    // if we have committed the operation, then safely ignore it
    if(view_stamp_comp(&msg->msg_vs,&comp->highest_committed_vs)<=0){
        goto handle_accept_req_exit;
    }else{
        // update highest seen request
        if(view_stamp_comp(&msg->msg_vs,&comp->highest_seen_vs)>0){
            comp->highest_seen_vs = msg->msg_vs;
        }
        // update highest requests that can be executed
        //
        SYS_LOG(comp,"Now Node %d Sees Request %u : %u .\n",
                comp->node_id,
                msg->req_canbe_exed.view_id,
                msg->req_canbe_exed.req_id);

        if(view_stamp_comp(&msg->req_canbe_exed,
                    &comp->highest_to_commit_vs)>0){

            comp->highest_to_commit_vs = msg->req_canbe_exed;
            SYS_LOG(comp,"Now Node %d Can Execute Request %u : %u .\n",
                    comp->node_id,
                    comp->highest_to_commit_vs.view_id,
                    comp->highest_to_commit_vs.req_id);
        }

        db_key_type record_no = vstol(&msg->msg_vs);
        request_record* origin_data = (request_record*)msg->data;
        request_record* record_data = (request_record*)malloc(
                REQ_RECORD_SIZE(origin_data));
        if(record_data==NULL){
            goto handle_accept_req_exit;
        }
        gettimeofday(&record_data->created_time,NULL);
        record_data->is_closed = origin_data->is_closed;
        record_data->data_size = origin_data->data_size;
        memcpy(record_data->data,origin_data->data,
                origin_data->data_size);

        // record the data persistently 
        if(store_record(comp->db_ptr,sizeof(record_no),&record_no,
                    REQ_RECORD_SIZE(record_data),record_data)!=0){
            goto handle_accept_req_exit;
        }
        // build the reply to the leader
        accept_ack* reply = build_accept_ack(comp,&msg->msg_vs);
        if(NULL==reply){
            goto handle_accept_req_exit;
        }
        comp->uc(comp->my_node,ACCEPT_ACK_SIZE,reply,msg->node_id);
        free(reply);
    }
handle_accept_req_exit:
    try_to_execute(comp);
    return;
};
static void handle_accept_ack(consensus_component* comp,void* data){
    accept_ack* msg = data;
    // if currently the node is not the leader, then it should ignore all the
    // accept ack, because that can must be the msg from previous view
    SYS_LOG(comp,"Node %d Handle Accept Ack From Node %u.\n",
            comp->node_id,msg->node_id);
    if(comp->my_role!=LEADER){
        goto handle_accept_ack_exit;
    }
    // the request has reached quorum
    if(view_stamp_comp(&msg->msg_vs,&comp->highest_committed_vs)<=0){
        goto handle_accept_ack_exit;
    }
    db_key_type record_no = vstol(&msg->msg_vs);
    request_record* record_data = NULL;
    size_t data_size;
    retrieve_record(comp->db_ptr,sizeof(record_no),&record_no,&data_size,(void**)&record_data);
    if(record_data==NULL){
        SYS_LOG(comp,"Received Ack To Non-Exist Record %lu.\n",
                record_no);
        goto handle_accept_ack_exit;
    }
    update_record(record_data,msg->node_id);
    // we do not care about whether the update is successful, otherwise this can
    // be treated as a message loss
    store_record(comp->db_ptr,sizeof(record_no),&record_no,REQ_RECORD_SIZE(record_data),record_data);
handle_accept_ack_exit:
    try_to_execute(comp);
    return;
};

static void handle_missing_req(consensus_component* comp,void* data){
    
    SYS_LOG(comp,"Node %d Handle Missing Req.\n",
            comp->node_id);
    missing_req* msg = data;

    SYS_LOG(comp,"Handle Missing Req %u : %u From Node %d.\n",
    msg->missing_vs.view_id,msg->missing_vs.req_id,msg->node_id);

    missing_ack* reply = build_missing_ack(comp,&msg->missing_vs);

    if(NULL!=reply){
        SYS_LOG(comp,"Send Missing Ack %u : %u To Node %d.\n",
            msg->missing_vs.view_id,msg->missing_vs.req_id,msg->node_id);
        comp->uc(comp->my_node,MISSING_ACK_SIZE(reply),reply,msg->node_id);
        free(reply);
    }
    
    return;
};
static void handle_missing_ack(consensus_component* comp,void* data){
    
    missing_ack* msg = data;
    request_record* origin = (request_record*)msg->data;
    SYS_LOG(comp,"Node %d Handle Missing Ack From Node %d.\n",
            comp->node_id,msg->node_id);
    if(view_stamp_comp(&comp->highest_committed_vs,&msg->missing_vs)>=0){
        goto handle_missing_ack_exit;
    }else{
       db_key_type record_no = vstol(&msg->missing_vs);
       request_record* record_data = NULL;
       size_t data_size;
       retrieve_record(comp->db_ptr,sizeof(record_no),&record_no,&data_size,(void**)&record_data);

       if(record_data!=NULL){
           goto handle_missing_ack_exit;
       }

       record_data =(request_record*)malloc(REQ_RECORD_SIZE(origin));

       if(record_data==NULL){
           goto handle_missing_ack_exit;
       }

       gettimeofday(&record_data->created_time,NULL);
       record_data->data_size = origin->data_size;
       memcpy(record_data->data,origin->data,origin->data_size);
       store_record(comp->db_ptr,sizeof(record_no),&record_no,REQ_RECORD_SIZE(record_data),record_data);
    }
    try_to_execute(comp);
handle_missing_ack_exit:
    
    return;
};
static void handle_force_exec(consensus_component* comp,void* data){
    force_exec* msg = data;
    if(msg->node_id!=comp->cur_view->leader_id){
        goto handle_force_exec_exit;
    }
    if(view_stamp_comp(&comp->highest_to_commit_vs,&msg->highest_committed_op)<0){
        comp->highest_to_commit_vs=msg->highest_committed_op;
        try_to_execute(comp);
    }
handle_force_exec_exit:
    return;
};



static void handle_forward_req(consensus_component* comp,void* data){
    
    if(comp->my_role!=LEADER){goto handle_forward_req_exit;}
    forward_req* msg = data;
    leader_handle_submit_req(comp,msg->data_size,msg->data,NULL);
handle_forward_req_exit:
    
    return;
}

static void* build_accept_req(consensus_component* comp,
        size_t data_size,void* data,view_stamp* vs){
    accept_req* msg = (accept_req*)malloc(sizeof(accept_req)+data_size);
    if(NULL!=msg){
        msg->node_id = comp->node_id;
        msg->req_canbe_exed.view_id = comp->highest_to_commit_vs.view_id;
        msg->req_canbe_exed.req_id = comp->highest_to_commit_vs.req_id;
        SYS_LOG(comp,"Now Node %d Give Execute Request %u : %u.\n",
                comp->node_id,
                comp->highest_to_commit_vs.view_id,
                comp->highest_to_commit_vs.req_id);
        msg->data_size = data_size;
        msg->header.msg_type = ACCEPT_REQ;
        msg->msg_vs = *vs;
        memcpy(msg->data,data,data_size);
    }
    return msg;
};

static void* build_accept_ack(consensus_component* comp,
        view_stamp* vs){
    accept_ack* msg = (accept_ack*)malloc(ACCEPT_ACK_SIZE);
    if(NULL!=msg){
        msg->node_id = comp->node_id;
        msg->msg_vs = *vs;
        msg->header.msg_type = ACCEPT_ACK;
    }
    return msg;
};

static void* build_missing_req(consensus_component* comp,view_stamp* vs){
    missing_req* msg = (missing_req*)malloc(MISSING_REQ);
    if(NULL!=msg){
        msg->header.msg_type = MISSING_REQ;
        msg->missing_vs.view_id = vs->view_id;
        msg->missing_vs.req_id = vs->req_id;

        msg->node_id = comp->node_id;
        SYS_LOG(comp,"In Building Req, The View Stamp Is %u : %u.\n",
                msg->missing_vs.view_id,msg->missing_vs.req_id);
    }
    return msg;
};

static void* build_missing_ack(consensus_component* comp,view_stamp* vs){
    missing_ack* msg = NULL;
    SYS_LOG(comp,"In Missing Ack, The View Stamp Is %u : %u.\n",
            vs->view_id,vs->req_id);
    db_key_type record_no = vstol(vs);
    request_record* record_data = NULL;
    size_t data_size;
    retrieve_record(comp->db_ptr,sizeof(record_no),&record_no,&data_size,(void**)&record_data);
    if(NULL!=record_data){
        int memsize = MISSING_ACK_SIZE(record_data);
        msg=(missing_ack*)malloc(memsize);
        if(NULL!=msg){
            msg->node_id = comp->node_id;
            msg->data_size = memsize;
            msg->header.msg_type = MISSING_ACK;
            memcpy(msg->data,record_data,memsize);
            msg->missing_vs = *vs;
        }
    }
    return msg;
};
static void* build_force_exec(consensus_component* comp){
    force_exec* msg = (force_exec*)malloc(FORCE_EXEC_SIZE);
    if(NULL!=msg){
        msg->header.msg_type = FORCE_EXEC;
        msg->node_id = comp->node_id;
        msg->highest_committed_op = comp->highest_committed_vs;
    }
    return msg;
};

static void* build_forward_req(consensus_component* comp,
        size_t data_size,void* data){
    
    forward_req* msg = (forward_req*)malloc(sizeof(forward_req)+data_size);
    if(NULL!=msg){
        msg->header.msg_type = FORWARD_REQ;
        msg->node_id = comp->node_id;
        msg->data_size = data_size;
        memcpy(msg->data,data,data_size);
    }
    
    return msg;
}

// leader has another responsibility to update the highest request that can be executed,
// and if the leader is also synchronous, it can execute the record in this stage
static void leader_try_to_execute(consensus_component* comp){
    db_key_type start = vstol(&comp->highest_to_commit_vs)+1;
    db_key_type end = vstol(&comp->highest_seen_vs);
    int exec_flag = (!view_stamp_comp(&comp->highest_committed_vs,&comp->highest_to_commit_vs));
    request_record* record_data = NULL;
    size_t data_size;
    SYS_LOG(comp,"The Leader Tries To Execute.\n");
    SYS_LOG(comp,"The End Value Is %lu.\n",end);
    for(db_key_type index=start;index<=end;index++){
        retrieve_record(comp->db_ptr,sizeof(index),&index,&data_size,(void**)&record_data);
        assert(record_data!=NULL && "The Record Should Be Inserted By The Node Itself!");
        if(reached_quorum(record_data,comp->group_size)){
            view_stamp temp = ltovs(index);
            SYS_LOG(comp,"Node %d : View Stamp %u : %u Has Reached Quorum.\n",
            comp->node_id,temp.view_id,temp.req_id);
            
            SYS_LOG(comp,"Before Node %d Inc Execute  %u : %u.\n",
                    comp->node_id,
                    comp->highest_to_commit_vs.view_id,
                    comp->highest_to_commit_vs.req_id);
            view_stamp_inc(&comp->highest_to_commit_vs);
            SYS_LOG(comp,"After Node %d Inc Execute  %u : %u.\n",
                    comp->node_id,
                    comp->highest_to_commit_vs.view_id,
                    comp->highest_to_commit_vs.req_id);

            if(exec_flag){
                view_stamp vs = ltovs(index);
                deliver_msg_data(comp,&vs);
                view_stamp_inc(&comp->highest_committed_vs);
            }
        }else{
            return;
        }
    }
}

static void try_to_execute(consensus_component* comp){
    // there we have assumption, for the currently leader,whose commited request
    // and highest request to execute must be in the same view, otherwise, the
    // leader cannot be the leader 
    
    SYS_LOG(comp,"Node %d Try To Execute.\n",
            comp->node_id);
    if(comp->cur_view->view_id==0){
        SYS_LOG(comp,"Node %d Currently Is A NULL Node\n",
                comp->node_id);
        goto try_to_execute_exit;
    }
    if(LEADER==comp->my_role){
        leader_try_to_execute(comp);
    }
    db_key_type start = vstol(&comp->highest_committed_vs)+1;
    db_key_type end;
    view_boundary* boundary_record = NULL;
    size_t data_size;
    if(comp->highest_committed_vs.view_id!=comp->highest_to_commit_vs.view_id){
        //address the boundary
        view_stamp bound;
        bound.view_id = comp->highest_committed_vs.view_id+1;
        bound.req_id = 0;
        db_key_type bound_record_no = vstol(&bound);
        retrieve_record(comp->db_ptr,sizeof(bound_record_no),&bound_record_no,&data_size,(void**)&boundary_record);
        if(NULL==boundary_record){
           send_missing_req(comp,&bound); 
           goto try_to_execute_exit;
        }
        end = vstol(&boundary_record->last_boundary);
    }else{
        end = vstol(&comp->highest_to_commit_vs);
    }
    SYS_LOG(comp,"The End Value Is %lu.\n",
           end);
    request_record* record_data = NULL;
    // we can only execute thins in sequence
    int exec_flag = 1;
    view_stamp missing_vs;
    for(db_key_type index = start;index<=end;index++){
        missing_vs = ltovs(index);
        if(0!=retrieve_record(comp->db_ptr,sizeof(index),&index,
                    &data_size,(void**)&record_data)){
            exec_flag = 0;
            send_missing_req(comp,&missing_vs);
        }
        if(exec_flag){
            deliver_msg_data(comp,&missing_vs);
//            record_data->is_closed = 1;
//            store_record(comp->db_ptr,sizeof(index),&index,REQ_RECORD_SIZE(record_data),record_data);
            view_stamp_inc(&comp->highest_committed_vs);
        }
        record_data = NULL;
    }
    if(NULL!=boundary_record){
        db_key_type op1 = vstol(&comp->highest_committed_vs);
        db_key_type op2 = vstol(&boundary_record->last_boundary);
        if(op1==op2){
            cross_view(&comp->highest_committed_vs);
        }
    }
try_to_execute_exit:
    return;
};

static void send_missing_req(consensus_component* comp,view_stamp* vs){
       missing_req* msg = build_missing_req(comp,vs);
       if(NULL==msg){
           goto send_for_consensus_comp_exit;
       }
       assert(comp->uc!=NULL&&"No Up Call Handler\n");
       comp->uc(comp->my_node,MISSING_REQ_SIZE,msg,-1);
       free(msg);
send_for_consensus_comp_exit:
       return;
};

static void cross_view(view_stamp* vs){
    vs->view_id++;
    vs->req_id = 0;
    return;
};

void consensus_make_progress(struct consensus_component_t* comp){
    if(LEADER!=comp->my_role){
        goto make_progress_exit;
    }
    leader_try_to_execute(comp);
    SYS_LOG(comp,"Let's Make Progress.\n");
    if((view_stamp_comp(&comp->highest_committed_vs,&comp->highest_seen_vs)<0)&& (comp->highest_seen_vs.view_id==comp->cur_view->view_id)){
        view_stamp temp;
        temp.view_id = comp->cur_view->view_id;
        temp.req_id = 0;
        if(view_stamp_comp(&temp,&comp->highest_committed_vs)<0){
            temp = comp->highest_committed_vs;
        }
        temp.req_id++;
        record_index_type start =  vstol(&temp);
        record_index_type end = vstol(&comp->highest_seen_vs);
        for(record_index_type index = start;index<=end;index++){
            request_record* record_data = NULL;
            size_t data_size=0;
            view_stamp temp_vs = ltovs(index);
            retrieve_record(comp->db_ptr,sizeof(db_key_type),&index,&data_size,(void**)&record_data);
            if(!reached_quorum(record_data,comp->group_size)){
                accept_req* msg = build_accept_req(comp,REQ_RECORD_SIZE(record_data),record_data,&temp_vs);
                if(NULL==msg){
                    continue;
                }else{
                    comp->uc(comp->my_node,ACCEPT_REQ_SIZE(msg),msg,-1);
                    free(msg);
                }
            }
        } 
    }
    force_exec* msg = build_force_exec(comp); 
    if(NULL==msg){goto make_progress_exit;}
    comp->uc(comp->my_node,FORCE_EXEC_SIZE,msg,-1);
    free(msg);
make_progress_exit:
    return;
};

static void deliver_msg_data(consensus_component* comp,view_stamp* vs){

    // in order to accelerate deliver process of the program
    // we may just give the record number instead of the real data 
    // to the proxy, and then the proxy will take the overhead of database operation
    
    db_key_type vstokey = vstol(vs);
    if(comp->deliver_mode==1)
    {
        request_record* data = NULL;
        size_t data_size=0;
        retrieve_record(comp->db_ptr,sizeof(db_key_type),&vstokey,&data_size,(void**)&data);
        SYS_LOG(comp,"Node %d Deliver View Stamp %u : %u To The User.\n",
        comp->node_id,vs->view_id,vs->req_id);
        STAT_LOG(comp,"Request:%lu\n",vstokey);
        if(NULL!=data){
            if(comp->ucb!=NULL){
                comp->ucb(data->data_size,data->data,comp->up_para);
            }else{
                SYS_LOG(comp,"No Such Call Back Func.\n");
            }
        }
    }else{
        STAT_LOG(comp,"Request %lu.\n",vstokey);
        if(comp->ucb!=NULL){
            comp->ucb(sizeof(db_key_type),&vstokey,comp->up_para);
        }else{
            SYS_LOG(comp,"No Such Call Back Func.\n");
        }
    }
    return;
}

