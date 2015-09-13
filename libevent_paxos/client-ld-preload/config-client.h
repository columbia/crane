#ifndef CONFIG_CLIENT_H
#define CONFIG_CLIENT_H
#include "../src/include/util/common-header.h"
#include "../src/include/replica-sys/node.h"
#include <libconfig.h>


extern int group_size;
extern peer* peer_pool;

extern int client_read_config();
/*
extern int get_group_size();

extern char *get_node_ip(int node_id);

extern int get_node_port(int node_id);*/

#endif

