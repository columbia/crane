/*
 * =====================================================================================
 *
 *       Filename:  config-proxy.h
 *        Version:  1.0
 *        Created:  09/16/2014 03:43:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef CONFIG_PROXY_H
#define CONFIG_PROXY_H

struct proxy_node_t;

int proxy_read_config(struct proxy_node_t* cur_node,const char* config_path);

#endif
