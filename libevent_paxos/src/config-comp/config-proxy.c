/*
 * =====================================================================================
 *
 *       Filename:  config-proxy.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/16/2014 04:44:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */


#include "../include/util/common-header.h"
#include "../include/proxy/proxy.h"
#include <libconfig.h>


int proxy_read_config(struct proxy_node_t* cur_node,const char* config_path){
    config_t config_file;
    config_init(&config_file);

    if(!config_read_file(&config_file,config_path)){
        goto goto_config_error;
    }
    
    uint32_t group_size;
    if(!config_lookup_int(&config_file,"group_size",(int*)&group_size)){
        goto goto_config_error;
    }

    if(group_size<=cur_node->node_id){
        err_log("PROXY : Invalid Node Id\n");
        goto goto_config_error;
    }

// parse proxy address

    config_setting_t *proxy_global_config = NULL;
    proxy_global_config = config_lookup(&config_file,"proxy_global_config");
    
    if(NULL!=proxy_global_config){
        long long temp;
        if(config_setting_lookup_int64(proxy_global_config,"reconnect_timeval_s",&temp)){
            cur_node->recon_period.tv_sec = temp;
        }
        if(config_setting_lookup_int64(proxy_global_config,"reconnect_timeval_us",&temp)){
            cur_node->recon_period.tv_usec = temp;
        }
        if(config_setting_lookup_int64(proxy_global_config,"action_timeval_s",&temp)){
            cur_node->action_period.tv_sec = temp;
        }
        if(config_setting_lookup_int64(proxy_global_config,"action_timeval_us",&temp)){
            cur_node->action_period.tv_usec = temp;
        }
        int sched_with_dmt;
        if(config_setting_lookup_int(proxy_global_config,"sched_with_dmt",&sched_with_dmt)){
            cur_node->sched_with_dmt = sched_with_dmt;
        }
    }

    config_setting_t *proxy_config = NULL;
    proxy_config = config_lookup(&config_file,"proxy_config");

    if(NULL==proxy_config){
        err_log("PROXY : Cannot Find Nodes Settings.\n");
        goto goto_config_error;
    }    

    config_setting_t *pro_ele = config_setting_get_elem(proxy_config,cur_node->node_id);

//    err_log("PROXY : Current Node Id Is %u.\n",cur_node->node_id);

    if(NULL==pro_ele){
        err_log("PROXY : Cannot Find Current Node's Address Section.\n");
        goto goto_config_error;
    }

// read the option for log, if it has some sections
    
    config_setting_lookup_int(pro_ele,"time_stamp_log",&cur_node->ts_log);
    config_setting_lookup_int(pro_ele,"sys_log",&cur_node->sys_log);
    config_setting_lookup_int(pro_ele,"stat_log",&cur_node->stat_log);
    config_setting_lookup_int(pro_ele,"req_log",&cur_node->req_log);

    const char* peer_ipaddr=NULL;
    int peer_port=-1;

    if(!config_setting_lookup_string(pro_ele,"ip_address",&peer_ipaddr)){
        err_log("PROXY : Cannot Find Current Node's IP Address.\n")
        goto goto_config_error;
    }
//    err_log("PROXY : Current Node's Address Is %s.\n",peer_ipaddr);
    if(!config_setting_lookup_int(pro_ele,"port",&peer_port)){
        err_log("PROXY : Cannot Find Current Node's Port.\n")
        goto goto_config_error;
    }

//    err_log("PROXY : Current Node's Port Is %u.\n",peer_port);

    cur_node->sys_addr.p_addr.sin_port = htons(peer_port);
    cur_node->sys_addr.p_addr.sin_family = AF_INET;
    inet_pton(AF_INET,peer_ipaddr,&cur_node->sys_addr.p_addr.sin_addr);
    cur_node->sys_addr.p_sock_len = sizeof(cur_node->sys_addr.p_addr);

    const char* db_name;
    if(!config_setting_lookup_string(pro_ele,"db_name",&db_name)){
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


// parse consensus address

    config_setting_t *consensus_config;
    consensus_config = config_lookup(&config_file,"consensus_config");

    if(NULL==consensus_config){
        err_log("PROXY : Cannot Find Nodes Settings \n");
        goto goto_config_error;
    }    

    config_setting_t *con_ele = config_setting_get_elem(consensus_config,cur_node->node_id);
    if(NULL==con_ele){
        err_log("PROXY : cannot find current node's address\n");
        goto goto_config_error;
    }
    peer_ipaddr=NULL;
    peer_port=-1;

    if(!config_setting_lookup_string(con_ele,"ip_address",&peer_ipaddr)){
        goto goto_config_error;
    }
    if(!config_setting_lookup_int(con_ele,"port",&peer_port)){
        goto goto_config_error;
    }
    cur_node->sys_addr.c_addr.sin_port = htons(peer_port);
    cur_node->sys_addr.c_addr.sin_family = AF_INET;
    inet_pton(AF_INET,peer_ipaddr,&cur_node->sys_addr.c_addr.sin_addr);
    cur_node->sys_addr.c_sock_len = sizeof(cur_node->sys_addr.c_addr);

// parse server address

    config_setting_t *server_config;
    server_config = config_lookup(&config_file,"server_config");

    if(NULL==server_config){
        err_log("cannot find nodes settings \n");
        goto goto_config_error;
    }    

    if(NULL==server_config){
        err_log("cannot find node address section \n");
        goto goto_config_error;
    }
    config_setting_t *serv_ele = config_setting_get_elem(server_config,cur_node->node_id);
    if(NULL==con_ele){
        err_log("cannot find current node's address\n");
        goto goto_config_error;
    }

    peer_ipaddr=NULL;
    peer_port=-1;
    if(!config_setting_lookup_string(serv_ele,"ip_address",&peer_ipaddr)){
        goto goto_config_error;
    }
    if(!config_setting_lookup_int(serv_ele,"port",&peer_port)){
        goto goto_config_error;
    }

    cur_node->sys_addr.s_addr.sin_port = htons(peer_port);
    cur_node->sys_addr.s_addr.sin_family = AF_INET;
    inet_pton(AF_INET,peer_ipaddr,&cur_node->sys_addr.s_addr.sin_addr);

    cur_node->sys_addr.s_sock_len = sizeof(cur_node->sys_addr.s_addr);


    config_destroy(&config_file);
    return 0;

goto_config_error:
    err_log("%s:%d - %s\n", config_error_file(&config_file),
            config_error_line(&config_file), config_error_text(&config_file));
    config_destroy(&config_file);
    return -1;
}
