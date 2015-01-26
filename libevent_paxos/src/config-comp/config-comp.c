/*
 * =====================================================================================
 *
 *       Filename:  config.c
 * *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/31/2014 02:21:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../include/config-comp/config-comp.h"

int consensus_read_config(node* cur_node,const char* config_path){
    config_t config_file;
    config_init(&config_file);

    if(!config_read_file(&config_file,config_path)){
        goto goto_config_error;
    }
    uint32_t group_size;
    if(!config_lookup_int(&config_file,"group_size",(int*)&group_size)){
        goto goto_config_error;
    }

    cur_node->group_size = group_size;
    cur_node->peer_pool = (peer*)malloc(group_size*sizeof(peer));
    if(NULL==cur_node->peer_pool){
        goto goto_config_error;
    }

    if(group_size<=cur_node->node_id){
        err_log("CONSENSUS : Configuration Reading Error : Invalid Node Id.\n");
        goto goto_config_error;
    }

    config_setting_t *consensus_global_config = NULL;
    consensus_global_config = config_lookup(&config_file,"consensus_global_config");
    
    if(NULL!=consensus_global_config){
        long long temp;
        if(config_setting_lookup_int64(consensus_global_config,"progress_timeval_s",&temp)){
            cur_node->config.make_progress_timeval.tv_sec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"progress_timeval_us",&temp)){
            cur_node->config.make_progress_timeval.tv_usec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"reconnect_timeval_s",&temp)){
            cur_node->config.reconnect_timeval.tv_sec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"reconnect_timeval_us",&temp)){
            cur_node->config.reconnect_timeval.tv_usec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"ping_timeval_s",&temp)){
            cur_node->config.ping_timeval.tv_sec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"ping_timeval_us",&temp)){
            cur_node->config.ping_timeval.tv_usec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"expected_ping_timeval_s",&temp)){
            cur_node->config.expect_ping_timeval.tv_sec = temp;
        }
        if(config_setting_lookup_int64(consensus_global_config,"expected_ping_timeval_us",&temp)){
            cur_node->config.expect_ping_timeval.tv_usec = temp;
        }
    }

    config_setting_t *nodes_config;
    nodes_config = config_lookup(&config_file,"consensus_config");

    if(NULL==nodes_config){
        err_log("CONSENSUS : Cannot Find Nodes Settings.\n");
        goto goto_config_error;
    }    

    if(NULL==nodes_config){
        err_log("CONSENSUS : Cannot Find Net Address Section.\n");
        goto goto_config_error;
    }
    peer* peer_pool = cur_node->peer_pool;
    for(uint32_t i=0;i<group_size;i++){ 
        config_setting_t *node_config = config_setting_get_elem(nodes_config,i);
        if(NULL==node_config){
            err_log("CONSENSUS : Cannot Find Node%u's Address.\n",i);
            goto goto_config_error;
        }

        const char* peer_ipaddr;
        int peer_port;
        if(!config_setting_lookup_string(node_config,"ip_address",&peer_ipaddr)){
            goto goto_config_error;
        }
        if(!config_setting_lookup_int(node_config,"port",&peer_port)){
            goto goto_config_error;
        }
        peer_pool[i].my_node = cur_node;
        peer_pool[i].peer_id = i; 
        peer_pool[i].base = cur_node->base; 
        peer_pool[i].reconnect = NULL;
        peer_pool[i].active = 0;
        peer_pool[i].my_buff_event = NULL;
        peer_pool[i].sock_id = -1;
        peer_pool[i].peer_address = (struct sockaddr_in*)malloc(sizeof(struct
                    sockaddr_in));
        peer_pool[i].sock_len = sizeof(struct sockaddr_in);
        peer_pool[i].peer_address->sin_family =AF_INET;
        inet_pton(AF_INET,peer_ipaddr,&peer_pool[i].peer_address->sin_addr);
        peer_pool[i].peer_address->sin_port = htons(peer_port);

        if(i==cur_node->node_id){
            config_setting_lookup_int(node_config,"sys_log",&cur_node->sys_log);
            config_setting_lookup_int(node_config,"stat_log",&cur_node->stat_log);
            const char* db_name;
            if(!config_setting_lookup_string(node_config,"db_name",&db_name)){
                goto goto_config_error;
            }
            size_t db_name_len = strlen(db_name);
            cur_node->db_name = (char*)malloc(sizeof(char)*(db_name_len+1));
            if(cur_node->db_name==NULL){
                goto goto_config_error;
            }
            if(NULL==strncpy(cur_node->db_name,db_name,db_name_len)){
                free(cur_node->db_name);
                goto goto_config_error;
            }

            cur_node->db_name[db_name_len] = '\0';
            cur_node->my_address.sin_port = htons(peer_port);
            cur_node->my_address.sin_family = AF_INET;
            inet_pton(AF_INET,peer_ipaddr,&cur_node->my_address.sin_addr);
        }
    }

    config_destroy(&config_file);
    return 0;

goto_config_error:
    err_log("CONSENSUS : %s:%d - %s\n", config_error_file(&config_file),
            config_error_line(&config_file), config_error_text(&config_file));
    config_destroy(&config_file);
    return -1;
};
