//#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <tr1/unordered_map>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/circular_buffer.hpp>

#include "tern/runtime/paxos-op-queue.h"


#define ELEM_CAPACITY 10000
#define DELTA 100 // TBD: don't know why we couldn't get 100% of the shared mem.
#define SEG_NAME "/PAXOS_OP_QUEUE-"
#define CB_NAME "/CIRCULAR_BUFFER-"
#define NODE_ROLE "/NODE_ROLE-"
#define NODE_INT "/NODE_INT-"
#define LOCK_FILE_NAME "paxos_queue_file_lock"
//#define DEBUG_PAXOS_OP_QUEUE

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;

#ifdef DEBUG_PAXOS_OP_QUEUE
#define DPRINT std::cerr << "Pid " << getpid() << ", PSELF " << (unsigned)pthread_self() << ": "
#else
#define DPRINT if (false) std::cerr
#endif

/** TBD: CURRENT WE ASSUME THE SERVER HAS ONLY ONE PROCESS TALKING TO THE PROXY.
IF THE SERVER HAVE MULTIPLE PROCESSES AND EACH OF THEM THEY CONNECT TO  
THE PROXY, THE PAXOS_OP NEEDS TO HAVE A PID FIELD. OR WE NEED TO SEPARATE THE 
PAXOS OP QUEUE TO A "PER SERVER PROCESS" BASED.**/

const char *timebubble_sockpath = "/dev/shm/timebubble.sock";
const char *timebubble_tag = "TIME_BUBBLE";
const int timebubble_clk_len = 8;

std::string sharedMemPath;
std::string circularBufPath;
std::string nodeRolePath;
std::string nodeIntPath;
std::string lockFilePath;

void initPaths() {
  std::string homePath = "/dev/shm/";//getenv("HOME");
  std::string userName = getenv("USER");
  assert(userName != "");
  homePath = homePath + userName + "-";
  sharedMemPath = SEG_NAME + userName;
  circularBufPath = CB_NAME + userName;
  nodeRolePath = NODE_ROLE + userName;
  nodeIntPath = NODE_INT + userName;
  lockFilePath = homePath + LOCK_FILE_NAME;
}

typedef std::tr1::unordered_map<uint64_t, int> conn_id_to_server_sock;
typedef std::tr1::unordered_map<int, uint64_t> server_sock_to_conn_id;
typedef std::tr1::unordered_map<unsigned, int> server_port_to_tid;
typedef std::tr1::unordered_map<int, unsigned> tid_to_server_port;
conn_id_to_server_sock conn_id_map;
server_sock_to_conn_id server_sock_map;
server_port_to_tid server_port_map;
tid_to_server_port tid_map;
int binded_socket = -1; /** The socket in the bind() function. **/
int timebubble_sock = -1;

void conns_init() {
  conn_id_map.clear();
  server_sock_map.clear();
  server_port_map.clear();
  tid_map.clear();
}

/* For detecting whether the client side has closed the socket. */
int conns_conn_id_exist(int server_sock) {
  server_sock_to_conn_id::iterator it = server_sock_map.find(server_sock);
  //if (it == server_sock_map.end())
    //return 0; // Not established yet.
  assert(it != server_sock_map.end());
  uint64_t conn_id = it->second;
  conn_id_to_server_sock::iterator it2 = conn_id_map.find(conn_id);
  int ret = (it2 != conn_id_map.end());
  DPRINT << "conns_conn_id_exist conn: server_sock " << server_sock
    << ", conn id " << (unsigned long)conn_id << ", exist? " << ret << std::endl;
  return ret; 
}

uint64_t conns_get_conn_id(int server_sock) {
  server_sock_to_conn_id::iterator it = server_sock_map.find(server_sock);
  assert(it != server_sock_map.end());
  return it->second;
}

int conns_exist_by_conn_id(uint64_t conn_id) {
  return (conn_id_map.find(conn_id) != conn_id_map.end());
}

int conns_exist_by_server_sock(int server_sock) {
  return (server_sock_map.find(server_sock) != server_sock_map.end());
}
    
int conns_get_server_sock(uint64_t conn_id) {
  conn_id_to_server_sock::iterator it = conn_id_map.find(conn_id);
  assert(it != conn_id_map.end());
  return it->second;
}

/** When the proxy sends a PAXQ_CLOSE operation, the server thread uses this function
to erase the connection_id. Do not need to erase the server_sock_map because the server
thread should do this job when it calls a close() operation. (because a socket is closed at both sides). **/
void conns_erase_by_conn_id(uint64_t conn_id) {
  conn_id_to_server_sock::iterator it = conn_id_map.find(conn_id);
  assert(it != conn_id_map.end());
  int server_sock = it->second;
  conn_id_map.erase(it);

  //server_sock_to_conn_id::iterator it2 = server_sock_map.find(server_sock);
  //assert(it2 != server_sock_map.end());
  //server_sock_map.erase(it2);
}

/** When the server calls a close() operation, the server thread uses this function
to erase the server_sock. Do not need to erase the conn_id_map because the server
thread should do this job when it receives a PAXQ_CLOSE from the proxy side
(because a socket is closed at both sides). **/
void conns_erase_by_server_sock(int server_sock) {
  server_sock_to_conn_id::iterator it2 = server_sock_map.find(server_sock);
  assert(it2 != server_sock_map.end());
  uint64_t conn_id = it2->second;
  server_sock_map.erase(it2);

  //conn_id_to_server_sock::iterator it = conn_id_map.find(conn_id);
  //assert(it != conn_id_map.end());
  //conn_id_map.erase(it);
}
    
void conns_add_pair(uint64_t conn_id, int server_sock) {
  assert(conn_id_map.find(conn_id) == conn_id_map.end());
  conn_id_map[conn_id] = server_sock;
  assert(server_sock_map.find(server_sock) == server_sock_map.end());
  server_sock_map[server_sock] = conn_id;
  DPRINT << "conns_add_pair added pair: (connection_id "
    << (unsigned long)conn_id << ", server_sock " << server_sock 
    << "), now conns size " << conns_get_conn_id_num() << std::endl;
}

/** Because a socket is closed at both sides (proxy and server),
sometimes the conn_id_map and server_sock_map can have different
sizes. Because this function is called by the TimeAlgo (to check the paxos
operations from the proxy queue), so I choose to return the conn_id_map.
**/
size_t conns_get_conn_id_num() {
  return conn_id_map.size();
}

void conns_add_tid_port_pair(int tid, unsigned port, int socket) {
  //fprintf(stderr, "conns_add_tid_port_map insert pair (%d, %u)\n", tid, port);
  /** Heming: these two asserts are to guarantee an assumption that a server application only
  listens on one port, not multiple ports. Thus, a bind() operation of is called
  only once within a process. This assumption is reasonable in modern servers.
  **/
  assert(server_port_map.find(port) == server_port_map.end());
  assert(tid_map.find(tid) == tid_map.end());
  server_port_map[port] = tid;
  tid_map[tid] = port;
  assert(binded_socket == -1 && "One process should call only one bind().");
  binded_socket = socket;
}

int conns_is_binded_socket(int socket) {
  if (binded_socket == -1)
    return false;
  else
    return binded_socket == socket;
}

int conns_get_tid_from_port(unsigned port) {
  //FIXME: THIS ASSERTION MAY FAIL IF BIND() HAPPENS AFTER A CLIENTS CONNECT.
  fprintf(stderr, "conns_get_tid_from_port query from port %u\n", port);
  assert(server_port_map.find(port) != server_port_map.end());
  return server_port_map[port];
}

unsigned conns_get_port_from_tid(int tid) {
  assert(tid_map.find(tid) != tid_map.end());
  //fprintf(stderr, "conns_get_port_from_tid: pid %d, tid %d, return port %u\n", getpid(), tid, tid_map[tid]);
  unsigned port = tid_map[tid];
  //assert(port != 0);//TODO: THIS RET VALUE SHOULD NOT BE 0.
  return port;
}

void conns_print() {
#ifndef DEBUG_PAXOS_OP_QUEUE
  return;
#endif

  if (conns_get_conn_id_num() > 0) {
    struct timeval tnow;
    gettimeofday(&tnow, NULL);
    DPRINT << "conns_print connection pair: now time (" << tnow.tv_sec << "." << tnow.tv_usec
      << "), size " << conns_get_conn_id_num() << std::endl;
    conn_id_to_server_sock::iterator it = conn_id_map.begin();
    int i = 0;
    for (; it != conn_id_map.end(); it++, i++) {
      /*This assertion no longer holds because conn_id_map and server_sock_map
      can have different sizes (see conns_erase_*() functions). */
      //assert(server_sock_map.find(it->second) != server_sock_map.end());
      DPRINT << "conns_print connection pair[" << i << "]: connection_id " << (unsigned long)it->first
        << ", server sock " << it->second << std::endl;
    }
    DPRINT << endl << endl;
  }

  if (tid_map.size() > 0) {
    DPRINT << "conns_print tid <-> port pair size: " << tid_map.size() << std::endl;
    tid_to_server_port::iterator it = tid_map.begin();
    int i = 0;
    for (; it != tid_map.end(); it++, i++) {
      assert(server_port_map.find(it->second) != server_port_map.end());
      DPRINT << "conns_print tid <-> port pair[" << i << "]: tid " << it->first << ", port " << it->second << std::endl;
    }
    DPRINT << endl << endl;
  }
}

namespace bip = boost::interprocess;
typedef bip::allocator<paxos_op, bip::managed_shared_memory::segment_manager> ShmemAllocatorCB;
typedef boost::circular_buffer<paxos_op, ShmemAllocatorCB> MyCircularBuffer;
typedef bip::allocator<int, bip::managed_shared_memory::segment_manager> ShmemAllocatorInt;
bip::managed_shared_memory *segment = NULL;
MyCircularBuffer *circbuff = NULL;
bip::managed_shared_memory *nodeRoleSeg = NULL;
int *nodeRole = NULL;
int lockFileFd = 0;
const char *paxq_op_str[5] = {"PAXQ_INVALID", "PAXQ_CONNECT", "PAXQ_SEND", "PAXQ_CLOSE", "PAXQ_NOP"};
__thread int my_tid = -1;
int proxyPid = -1;

/** This function is called by the DMT runtime, because the eval.py starts it before the proxy. **/
void paxq_create_shared_mem() {
  initPaths();

  // Create the IPC circular buffer.
  bip::permissions perm;
  perm.set_unrestricted();
  std::cerr << "Init shared memory " << (bip::shared_memory_object::remove(sharedMemPath.c_str()) ?
    "cleaned up: " : "not exist: " ) << sharedMemPath.c_str() << "\n";
  segment = new bip::managed_shared_memory(bip::create_only, sharedMemPath.c_str(),
    (ELEM_CAPACITY+DELTA)*sizeof(paxos_op), 0, perm);
  static const ShmemAllocatorCB alloc_inst (segment->get_segment_manager());
  circbuff = segment->construct<MyCircularBuffer>(circularBufPath.c_str())(ELEM_CAPACITY, alloc_inst);
  circbuff->clear();
  assert(circbuff->size() == 0);
  paxq_print();

  // Create the IPC paxos role (current node is leader or not).
  bip::shared_memory_object::remove(nodeRolePath.c_str());
  nodeRoleSeg = new bip::managed_shared_memory(bip::create_only, nodeRolePath.c_str(),
    sizeof(int)*1024, 0, perm);
  nodeRole = nodeRoleSeg->construct<int>(nodeIntPath.c_str())(10);
  *nodeRole = ROLE_INVALID;
  
  // Create the IPC lock file.
  lockFileFd = open(lockFilePath.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  chmod(lockFilePath.c_str(), 0666);
  if (lockFileFd == -1) {
    std::cerr << "paxq_create_shared_memory file lock " << lockFilePath << " open failed, errno " << errno << ".\n";
    exit(1);
  }
}

/** This function is called by the proxy, because the eval.py starts the proxy after the server. . **/
void paxq_open_shared_mem() {
  initPaths();

  // Open the IPC circular buffer.
  segment = new bip::managed_shared_memory(bip::open_only, sharedMemPath.c_str());
  circbuff = segment->find<MyCircularBuffer>(circularBufPath.c_str()).first;

  // Open the IPC paxos role (current node is leader or not).
  nodeRoleSeg = new bip::managed_shared_memory(bip::open_only, nodeRolePath.c_str());
  nodeRole = nodeRoleSeg->find<int>(nodeIntPath.c_str()).first;
  assert(*nodeRole == ROLE_INVALID);

  // Open the IPC lock file.
  lockFileFd = open(lockFilePath.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
  chmod(lockFilePath.c_str(), 0666);
  if (lockFileFd == -1) {
    std::cerr << "paxq_open_shared_memory file lock " << lockFilePath << " open failed, errno " << errno << ".\n";
    exit(1);
  }
}

void paxq_update_role(int is_leader) {
  if (nodeRole) {// This pointer is not NULL only when sched_with_paxos is 1. 
    if (is_leader)
      *nodeRole = ROLE_LEADER;
    else
      *nodeRole = ROLE_SECONDARY;
  }
}

int paxq_role_is_leader() {
  int role = *nodeRole;
  assert(role != ROLE_INVALID);
  int ret = (role == ROLE_LEADER);
  DPRINT << "Current node is leader: " << ret << std::endl;
  return ret;
}
/*
void paxq_insert_front(int with_lock, uint64_t conn_id, uint64_t counter, PAXOS_OP_TYPE t, int value) {
  struct timeval tnow;
  gettimeofday(&tnow, NULL);
  if (with_lock) paxq_lock();
  if (paxq_size() == ELEM_CAPACITY) {
    std::cout << sharedMemPath << " is too small for this app. Please enlarge it in paxos-op-queue.h\n"; 
    if (with_lock) paxq_unlock();
    exit(1);
  }
  paxos_op op = {conn_id, counter, t, value}; // TBD: is this OK for IPC shared-memory?
  circbuff->insert(circbuff->begin(), op);   
  DPRINT << "paxq_insert_front time <" << tnow.tv_sec << "." << tnow.tv_usec
    << ">, now paxq size " << paxq_size() << "\n";
  if (t == PAXQ_NOP)
    DPRINT << "paxq_insert_front time <" << tnow.tv_sec << "." << tnow.tv_usec
    << "> inserted a timebubble with count: " << value << "\n";
  paxq_print();
  if (with_lock) paxq_unlock();
}
*/
void paxq_push_back(int with_lock, uint64_t conn_id, uint64_t counter, PAXOS_OP_TYPE t, int value) {
//#ifdef DEBUG_PAXOS_OP_QUEUE
  struct timeval tnow;
  gettimeofday(&tnow, NULL);
  DPRINT << "paxq_push_back time <" << tnow.tv_sec << "." << tnow.tv_usec
    << "> , op (" << (unsigned long)conn_id << ", " << counter << ", " << paxq_op_str[t] << ", " << value << ")." << std::endl;
//#endif

  if (with_lock) paxq_lock();
  if (paxq_size() == ELEM_CAPACITY) {
    std::cerr << sharedMemPath << " is too small for this app. Please enlarge it in paxos-op-queue.h\n"; 
    if (with_lock) paxq_unlock();
    exit(1);
  }
  paxos_op op = {conn_id, counter, t, value}; // TBD: is this OK for IPC shared-memory?
  circbuff->push_back(op);      
  if (with_lock) paxq_unlock();
}

int paxq_update_op_val(unsigned index, uint64_t new_val) {
  assert(index < paxq_size());
  assert((*circbuff)[index].value > new_val);
  DPRINT << "paxq_update_op_val: update value (bytes to read) in current op[" 
    << index << "] from "
    << (unsigned)(*circbuff)[index].value << " to " << (unsigned)new_val << "."
    << std::endl; 
  (*circbuff)[index].value = new_val;
  return 0;
}
paxos_op paxq_get_op(unsigned index) {
  assert(index < paxq_size());
  paxos_op ret = (*circbuff)[index];
  return ret;
}

int paxq_get_op2(unsigned index, paxos_op *op) {
  assert(index < paxq_size());
  paxos_op ret = (*circbuff)[index];
  op->connection_id = ret.connection_id;
  op->counter = ret.counter;
  op->type = ret.type;
  op->value = ret.value;
  return 1;
}

int paxq_dec_front_value() {
  assert(paxq_size() > 0);
  paxos_op &op = circbuff->front();
  assert(op.type == PAXQ_NOP);
  assert(op.value != 0);
  op.value--;
  return op.value;
}


paxos_op paxq_pop_front(int debugTag) {
  struct timeval tnow;
  gettimeofday(&tnow, NULL);
  paxos_op op = paxq_get_op(0);
  if (op.type == PAXQ_CONNECT)
    paxq_set_proxy_pid(op.counter);
  circbuff->pop_front();
  DPRINT << "DEBUG TAG " << debugTag
    << ": paxq_pop_front time <" << tnow.tv_sec << "." << tnow.tv_usec 
    << ">: (" << (unsigned long)op.connection_id
    << ", " << (unsigned long)op.counter << ", " << paxq_op_str[op.type]
    << ", " << op.value << ")" << std::endl;
  return op;
} 

size_t paxq_size() {
  size_t t = circbuff->size();
  return t;       
}

void paxq_lock() {
  int ret = lockf(lockFileFd, F_LOCK, 0);
}

void paxq_unlock() {
  int ret = lockf(lockFileFd, F_ULOCK, 0);
}

void paxq_set_proxy_pid(int pid) {
  if (proxyPid < 0) {
    proxyPid = pid;
    DPRINT << "Proxy Pid is " << proxyPid << std::endl;
  }
}

int paxq_build_timebubble_conn() {
  sleep(1);
  struct sockaddr_un address;
  DPRINT << "Server-app side timebubble connection: START!" << std::endl;
  timebubble_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  assert(timebubble_sock >= 0);
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, timebubble_sockpath);
  if (connect(timebubble_sock, (sockaddr*)&address, sizeof(address)) < 0) {
    DPRINT << "Server-app side timebubble connection: FAIL TO CONNECT!" << std::endl;
    return -1;
  }
  int enable = 1;
  if(setsockopt(timebubble_sock, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)) < 0) {
    DPRINT << "Server-app side timebubble connection: TCP_NODELAY SETTING ERROR!" << std::endl;
    return -1;
  }
  DPRINT << "DMT server pid " << getpid() << " builds timebubble connection with proxy." << std::endl;
  return 0;
}

// Just a connect and close is enough. Just a "signal".
// We use socket instead of tkill() because server app may run in lxc.
void paxq_notify_proxy(int timebubbleCnt) {
  int ret;
  const int buf_len = strlen(timebubble_tag) + timebubble_clk_len;
  char buf[buf_len+1];
  memset(buf, 0, buf_len+1);
  assert(timebubble_clk_len == 8);
  snprintf(buf, buf_len+1, "%08d%s", timebubbleCnt, timebubble_tag);
  if (timebubble_sock < 0) {
    ret = paxq_build_timebubble_conn();
    DPRINT << "DMT server pid init timebubble connection, result " << ret << std::endl;
    assert(ret == 0);
  }
  DPRINT << "paxq_notify_proxy sends timebubble req (" << buf << ") (" << 
  timebubbleCnt << ")." << std::endl;
  ret = write(timebubble_sock, buf, buf_len+1);
  if (ret == -1) {
    close(timebubble_sock);
    timebubble_sock = -1;
    DPRINT << "DMT-proxy timebubble connection is closed. Reset timebubble_sock for reconnect."
      << std::endl;
  }
  DPRINT << "DMT server pid " << getpid() << " asks proxy pid "
    << proxyPid << " for logical clocks (sock " << timebubble_sock
    << "), connect result (0 is good): " << ret << std::endl;
   assert(proxyPid > 0);
}
 

// Proxy calls this function, and it must grabs the paxq lock first.
void paxq_proxy_give_clocks(int timebubble_cnt) {
  paxq_lock();
  paxq_push_back(0, 0, 0, PAXQ_NOP, timebubble_cnt);
  /*assert(paxq_size() > 0 &&  && "paxq_proxy_give_clocks: PAXQ must contain a timebubble");
  paxos_op &op = (*circbuff)[0];
  assert(op.type == PAXQ_NOP && op.value < 0 &&
    "paxq_proxy_give_clocks: PAXQ head must be a timebubble");
  op.value = -1*op.value;
  paxos_op &op2 = (*circbuff)[0];
  assert(op2.value > 0);*/
#ifdef DEBUG_PAXOS_OP_QUEUE
  struct timeval tnow;
  gettimeofday(&tnow, NULL);
  fflush(stderr);
  fprintf(stderr, "Proxy pid %d, time <%ld.%ld>, give DMT clks %d.\n",
    getpid(), tnow.tv_sec, tnow.tv_usec, timebubble_cnt);
  fflush(stderr);
  //std::cerr << std::endl << "Proxy pid " << getpid()
    //<< ", now time (" << tnow.tv_sec << "." << tnow.tv_usec << "),"
    //<< " gives " << op2.value << " logical clocks to DMT." << std::endl;
#endif
  paxq_unlock();
}

void paxq_delete_ops(uint64_t conn_id, unsigned num_delete) {
  unsigned actual_delete = 0;
  while (actual_delete < num_delete) {
    assert(paxq_size() > 0);
    MyCircularBuffer::iterator itr = circbuff->begin();
    unsigned i = 0;
    for (; itr != circbuff->end(); itr++, i++) {
      paxos_op op = (*circbuff)[i];
      if (op.connection_id == conn_id) {
#ifdef DEBUG_PAXOS_OP_QUEUE
        struct timeval tnow;
        gettimeofday(&tnow, NULL);
        fprintf(stderr, "DEBUG TAG 1: paxq_pop_front time <%ld.%ld>: (%ld, %lu, %s, %d)\n",
          tnow.tv_sec, tnow.tv_usec, (long)op.connection_id, (unsigned long)op.counter, paxq_op_str[op.type], op.value);
#endif
        circbuff->erase(itr);
        actual_delete++;
        break;
      }
    }
  }
  DPRINT << "paxq_delete_ops deleted number of operations: " << actual_delete << std::endl;
}

void paxq_set_conn_id(unsigned index, uint64_t new_conn_id) {
  assert(index < paxq_size());
  (*circbuff)[index].connection_id = new_conn_id;
}

int paxq_gettid() {
  if (my_tid == -1)
    return my_tid = syscall(SYS_gettid);
  else
    return my_tid;
}

void paxq_test() {
#ifndef DEBUG_PAXOS_OP_QUEUE
  return;
#endif


  std::cerr << "Circular Buffer Size before push_back: " << circbuff->size() << "\n";
  for (int i = 0; i < ELEM_CAPACITY*2+123; i++) {
    paxos_op op = {i, i, PAXQ_SEND};
    circbuff->push_back(op);
    //push_back(i, i, SEND); This code will trigger the circular buffer 
    // full exit, which is good.
  }
  std::cerr << "Circular Buffer Size after push_back: " << circbuff->size() << "\n";

  for (int i = 0; i < 10; i++) {
    std::cerr << "Child got: " << (unsigned long)(*circbuff)[i].connection_id
      << ", " << (unsigned long)(*circbuff)[i].counter << ", " << (*circbuff)[i].type << "\n";
  }

  std::cerr << "\n\nCircular Buffer Size after pop: " << circbuff->size() << "\n";
  for (int i = 0; i < 10; i++) {
    std::cerr << "Child got: " << (unsigned long)(*circbuff)[i].connection_id
      << ", " << (unsigned long)(*circbuff)[i].counter << "\n";
  }  
}

void paxq_print() {
#ifndef DEBUG_PAXOS_OP_QUEUE
  return;
#endif

  if (paxq_size() == 0)
    return;
  //boost::circular_buffer::iterator itr = circbuff->begin();
  //std::cerr << "paxq_print circbuff now " << circbuff << ", pself " << pthread_self() << std::endl;
  struct timeval tnow;
  gettimeofday(&tnow, NULL);
  std::cerr << "paxq_print size now time (" << tnow.tv_sec << "." << tnow.tv_usec << "), size "
    << paxq_size() << std::endl;
  for (size_t i = 0; i < paxq_size(); i++) {
    paxos_op &op = (*circbuff)[i];
    std::cerr << "paxq_print [" << i << "]: (" << (unsigned long)op.connection_id
    << ", " << (unsigned long)op.counter << ", " << paxq_op_str[op.type] << ", " << op.value << ")\n";
  }
}

#ifdef __cplusplus
}
#endif

