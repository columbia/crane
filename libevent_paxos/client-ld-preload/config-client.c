#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include "config-client.h"

int group_size;
peer* peer_pool;

//Init group size and peer_pool, get IPs.
int client_read_config() {
  return 0; // Disable it for now. Do the testing framework first.
  char config_path[1024] = {0};
  const char *proj_root_path = getenv("MSMR_ROOT");
  sprintf(config_path, "%s/%s", proj_root_path, "eval-container/nodes.local.cfg");

  config_t config_file;
  config_init(&config_file);

  if(!config_read_file(&config_file,config_path))
    goto goto_config_error;
  if(!config_lookup_int(&config_file,"group_size",(int*)&group_size))
    goto goto_config_error;

  peer_pool = (peer*)malloc(group_size*sizeof(peer));
  if (!peer_pool)
    goto goto_config_error;

    config_setting_t *proxy_config = NULL;
    proxy_config = config_lookup(&config_file,"proxy_config");

    for (unsigned i=0;i < group_size; i++) { 
      config_setting_t *node_config = config_setting_get_elem(proxy_config,i);
      if(NULL==node_config){
          err_log("CONSENSUS : Cannot Find Node%u's Address.\n", i);
          goto goto_config_error;
      }

      const char* peer_ipaddr;
      int peer_port;
      if(!config_setting_lookup_string(node_config,"ip_address",&peer_ipaddr))
          goto goto_config_error;
      if(!config_setting_lookup_int(node_config,"port",&peer_port))
          goto goto_config_error;
      fprintf(stderr, "Node %u IP %s, port %d\n", i, peer_ipaddr, peer_port);
      peer_pool[i].peer_address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
      peer_pool[i].sock_len = sizeof(struct sockaddr_in);
      peer_pool[i].peer_address->sin_family =AF_INET;
      inet_pton(AF_INET,peer_ipaddr,&peer_pool[i].peer_address->sin_addr);
      peer_pool[i].peer_address->sin_port = htons(peer_port);
  }

  config_destroy(&config_file);
  return 0;
goto_config_error:
  config_destroy(&config_file);
  return -1;
}
/*
int get_group_size() {
  return group_size;
}

char *get_node_ip(int node_id) {
  assert(node_id >= 0 && node_id < group_size);
  return NULL;
}

int get_node_port(int node_id) {
  assert(node_id >= 0 && node_id < group_size);
  return ntohs(peer_pool[node_id].peer_address->sin_port);
}*/

