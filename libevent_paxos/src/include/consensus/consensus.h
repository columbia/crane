/*
 * =====================================================================================
 *
 *       Filename:  consensus.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/06/2014 02:09:10 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef CONSENSUS_H

#define CONSENSUS_H
#include "../util/common-header.h"

typedef uint64_t db_key_type;
struct node_t;
struct consensus_component_t;


typedef struct view_boundary_t{
    view_id_t view_id;
    req_id_t req_id;
    node_id_t leader_id;
}view_boundary;
#define VIEW_BOUNDARY_SIZE (sizeof(view_boundary))


typedef void (*up_call)(struct node_t*,size_t,void*,int);
typedef void (*user_cb)(size_t data_size,void* data,void* arg);

typedef enum con_role_t{
    LEADER = 0,
    SECONDARY = 1,
}con_role;

struct consensus_component_t* init_consensus_comp(struct node_t*,uint32_t,FILE*,int,int,
        const char*,int,void*,int,
        view*,view_stamp*,view_stamp*,view_stamp*,user_cb,up_call,void*);

void consensus_handle_msg(struct consensus_component_t*,size_t,void*);

int consensus_submit_request(struct consensus_component_t*,size_t,void*,view_stamp*);

void consensus_make_progress(struct consensus_component_t*);

int consensus_look_up_request(struct consensus_component_t*,view_stamp);

view_stamp consensus_get_highest_seen_req(struct consensus_component_t*);

void consensus_update_role(struct consensus_component_t*);

#endif
