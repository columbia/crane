/* Copyright (c) 2013,  Regents of the Columbia University 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __TERN_COMMON_PAXOS_OP_QUEUE_H
#define __TERN_COMMON_PAXOS_OP_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_PORT 39999 /* For apache, because the worker process directly 
blocks on accept without calling bind() (its parent calls it).*/

typedef enum {
  PAXQ_INVALID = 0,
  PAXQ_CONNECT,
  PAXQ_SEND,
  PAXQ_CLOSE,
  PAXQ_NOP
} PAXOS_OP_TYPE;

typedef enum {
  ROLE_INVALID = 0,
  ROLE_SECONDARY = 1,
  ROLE_LEADER = 2,
} NODE_ROLE;

typedef struct {
  uint64_t connection_id;
  uint64_t counter;
  PAXOS_OP_TYPE type;
  int value; /** This field means "port" for PAXQ_CONNECT, "logical clock" 
  for PAXQ_NOP, and number of actual bytes sent for PAXQ_SEND. **/
} paxos_op;  


/// APIs for the proxy-server two-way mapping connections.
void conns_init();
int conns_conn_id_exist(int server_sock);
uint64_t conns_get_conn_id(int server_sock);
int conns_get_server_sock(uint64_t conn_id);
int conns_exist_by_conn_id(uint64_t conn_id);
int conns_exist_by_server_sock(int server_sock);
void conns_erase_by_conn_id(uint64_t conn_id);
void conns_erase_by_server_sock(int server_sock);
void conns_add_pair(uint64_t conn_id, int server_sock);
size_t conns_get_conn_id_num();
void conns_add_tid_port_pair(int tid, unsigned port, int socket);
int conns_is_binded_socket(int socket);
int conns_get_tid_from_port(unsigned port);
unsigned conns_get_port_from_tid(int tid);
void conns_print();


/// APIs for the proxy-server paxos operation queue.
void paxq_create_shared_mem();
void paxq_open_shared_mem();
void paxq_update_role(int is_leader);
int paxq_role_is_leader();
//void paxq_insert_front(int with_lock, uint64_t conn_id, uint64_t counter, PAXOS_OP_TYPE t, int value);
void paxq_push_back(int with_lock, uint64_t conn_id, uint64_t counter, PAXOS_OP_TYPE t, int value);
int paxq_update_op_val(unsigned index, uint64_t dec);
paxos_op paxq_get_op(unsigned index);
int paxq_get_op2(unsigned index, paxos_op *op);
int paxq_dec_front_value();
paxos_op paxq_pop_front(int debugTag);
size_t paxq_size();
void paxq_lock();
void paxq_unlock();
void paxq_set_proxy_pid(int pid);
void paxq_notify_proxy(int timebubbleCnt);
int paxq_build_timebubble_conn();
void paxq_proxy_give_clocks(int timebubble_cnt);
void paxq_delete_ops(uint64_t conn_id, unsigned num_delete);
void paxq_set_conn_id(unsigned index, uint64_t new_conn_id);
int paxq_gettid();
//void paxq_tkill(int tid, int sig);


void paxq_test();
void paxq_print();

extern const char *timebubble_sockpath;
extern const char *timebubble_tag;
extern const int timebubble_clk_len;

#ifdef __cplusplus
}
#endif

#endif

