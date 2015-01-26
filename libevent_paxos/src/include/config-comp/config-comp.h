/*
 * ===================================================================================== *
 *       Filename:  config.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/29/2014 03:25:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef CONFIG_COMP_H
#define CONFIG_COMP_H
#include "../util/common-header.h"
#include "../replica-sys/node.h"
#include <libconfig.h>


extern int max_waiting_connections;

#define MAX_ACCEPT_CONNECTIONS 500

int consensus_read_config(node* cur_node,const char* config_file);

#endif
