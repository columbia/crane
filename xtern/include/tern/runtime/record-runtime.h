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

/* Authors: Heming Cui (heming@cs.columbia.edu), Junfeng Yang (junfeng@cs.columbia.edu) -*- Mode: C++ -*- */
// Refactored from Heming's Memoizer code

#ifndef __TERN_RECORDER_RUNTIME_H
#define __TERN_RECORDER_RUNTIME_H

#include <tr1/unordered_map>
#include "tern/runtime/runtime.h"
#include "tern/runtime/record-scheduler.h"
#include "runtime-stat.h"
#include <time.h>
#include "tern/runtime/paxos-op-queue.h"

namespace tern {

struct barrier_t {
  int count;    // barrier count
  int narrived; // number of threads arrived at the barrier
};
struct ref_cnt_barrier_t {
  unsigned count;    // barrier count
  unsigned nactive;  // number of threads in the linup region (between lineup_starnt and lineup_end).
  unsigned timeout;  // Number of turns that an operation (at most) has to block.
  enum PHASE {ARRIVING, LEAVING}; // ARRIVING: we have to wait up to "count" threads arrive or timeout.
                              // LEAVING: timeout has happened or all threads have arrived.
  PHASE phase;
  long nSuccess;
  long nTimeout;
  ref_cnt_barrier_t() {
    nSuccess = nTimeout = 0;
  }
  void setArriving() {phase = ARRIVING;}
  void setLeaving() {phase = LEAVING;}
  bool isArriving() {return phase == ARRIVING;}
  bool isLeaving() {return phase == LEAVING;}
};
typedef std::tr1::unordered_map<pthread_barrier_t*, barrier_t> barrier_map;
typedef std::tr1::unordered_map<unsigned, ref_cnt_barrier_t> refcnt_bar_map;

typedef std::tr1::unordered_map<pthread_t, int> tid_map_t;
typedef std::tr1::unordered_map<void*, std::list<int> > waiting_tid_t;

template <typename _Scheduler>
struct RecorderRT: public Runtime, public _Scheduler {

  void progBegin(void);
  void progEnd(void);
  void threadBegin(void);
  void threadEnd(unsigned insid);
  void idle_sleep();
  void idle_cond_wait();

  // thread
  int pthreadCreate(unsigned insid, int &error, pthread_t *thread,  pthread_attr_t *attr,
                    void *(*thread_func)(void*), void *arg);
  int pthreadJoin(unsigned insid, int &error, pthread_t th, void **thread_return);
  int __pthread_detach(unsigned insid, int &error, pthread_t th);

  // mutex
  int pthreadMutexInit(unsigned insid, int &error, pthread_mutex_t *mutex, const  pthread_mutexattr_t *mutexattr);
  int pthreadMutexDestroy(unsigned insid, int &error, pthread_mutex_t *mutex);
  int pthreadMutexLock(unsigned insid, int &error, pthread_mutex_t *mutex);
  int pthreadMutexTimedLock(unsigned insid, int &error, pthread_mutex_t *mutex,
                            const struct timespec *abstime);
  int pthreadMutexTryLock(unsigned insid, int &error, pthread_mutex_t *mutex);

  int pthreadMutexUnlock(unsigned insid, int &error, pthread_mutex_t *mutex);

  // cond var
  int pthreadCondWait(unsigned insid, int &error, pthread_cond_t *cv, pthread_mutex_t *mu);
  int pthreadCondTimedWait(unsigned insid, int &error, pthread_cond_t *cv,
                           pthread_mutex_t *mu, const struct timespec *abstime);
  int pthreadCondSignal(unsigned insid, int &error, pthread_cond_t *cv);
  int pthreadCondBroadcast(unsigned insid, int &error, pthread_cond_t *cv);

  // barrier
  int pthreadBarrierInit(unsigned insid, int &error, pthread_barrier_t *barrier, unsigned count);
  int pthreadBarrierWait(unsigned insid, int &error, pthread_barrier_t *barrier);
  int pthreadBarrierDestroy(unsigned insid, int &error, pthread_barrier_t *barrier);

  // semaphore
  int semInit(unsigned insid, int &error, sem_t *sem, int pshared, unsigned int value);
  int semWait(unsigned insid, int &error, sem_t *sem);
  int semTryWait(unsigned insid, int &error, sem_t *sem);
  int semTimedWait(unsigned insid, int &error, sem_t *sem, const struct timespec *abstime);
  int semPost(unsigned insid, int &error, sem_t *sem);

  // new programming primitives
  void lineupInit(long opaque_type, unsigned count, unsigned timeout_turns);
  void lineupDestroy(long opaque_type);
  void lineupStart(long opaque_type);
  void lineupEnd(long opaque_type);
  void nonDetStart();
  void nonDetEnd();
  void threadDetach();
  void threadDisableSchedPaxos();
  void nonDetBarrierEnd(int bar_id, int cnt);
  void setBaseTime(struct timespec *ts);
  
  void symbolic(unsigned insid, int &error, void *addr, int nbytes, const char *name);

  // socket & file
  bool socketFd(int fd);
  bool regularFile(int fd);
  int __socket(unsigned insid, int &error, int domain, int type, int protocol);
  int __listen(unsigned insid, int &error, int sockfd, int backlog);
  int __accept(unsigned insid, int &error, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);
  int __accept4(unsigned insid, int &error, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen, int flags);
  int __connect(unsigned insid, int &error, int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
  struct hostent *__gethostbyname(unsigned insid, int &error, const char *name);
  int __gethostbyname_r(unsigned insid, int &error, const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop);
  int __getaddrinfo(unsigned insid, int &error, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
  void __freeaddrinfo(unsigned insid, int &error, struct addrinfo *res);
  struct hostent *__gethostbyaddr(unsigned insid, int &error, const void *addr, int len, int type);
  char *__inet_ntoa(unsigned ins, int &error, struct in_addr in);
  char *__strtok(unsigned ins, int &error, char * str, const char * delimiters);
  ssize_t __send(unsigned insid, int &error, int sockfd, const void *buf, size_t len, int flags);
  ssize_t __sendto(unsigned insid, int &error, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
  ssize_t __sendmsg(unsigned insid, int &error, int sockfd, const struct msghdr *msg, int flags);
  ssize_t __recv(unsigned insid, int &error, int sockfd, void *buf, size_t len, int flags);
  ssize_t __recvfrom(unsigned insid, int &error, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
  ssize_t __recvmsg(unsigned insid, int &error, int sockfd, struct msghdr *msg, int flags);
  int __shutdown(unsigned insid, int &error, int sockfd, int how);
  int __getpeername(unsigned insid, int &error, int sockfd, struct sockaddr *addr, socklen_t *addrlen);  
  int __getsockopt(unsigned insid, int &error, int sockfd, int level, int optname, void *optval, socklen_t *optlen);
  int __setsockopt(unsigned insid, int &error, int sockfd, int level, int optname, const void *optval, socklen_t optlen);
  int __pipe(unsigned insid, int &error, int pipefd[2]);
  int __fcntl(unsigned insid, int &error, int fd, int cmd, void *arg);
  int __close(unsigned insid, int &error, int fd);
  ssize_t __read(unsigned insid, int &error, int fd, void *buf, size_t count);
  ssize_t __write(unsigned insid, int &error, int fd, const void *buf, size_t count);
  size_t __fread (unsigned insid, int &error, void * ptr, size_t size, size_t count, FILE * stream);
  ssize_t __pread(unsigned insid, int &error, int fd, void *buf, size_t count, off_t offset);
  ssize_t __pwrite(unsigned insid, int &error, int fd, const void *buf, size_t count, off_t offset);
  int __select(unsigned insid, int &error, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);  
  int __poll(unsigned insid, int &error, struct pollfd *fds, nfds_t nfds, int timeout);
  int __bind(unsigned insid, int &error, int socket, const struct sockaddr *address, socklen_t address_len);
  int __epoll_wait(unsigned insid, int &error, int epfd, struct epoll_event *events, int maxevents, int timeout);
  int __epoll_create(unsigned insid, int &error, int size);
  int __epoll_ctl(unsigned insid, int &error, int epfd, int op, int fd, struct epoll_event *event);
  int __sigwait(unsigned insid, int &error, const sigset_t *set, int *sig); 
  char *__fgets(unsigned insid, int &error, char *s, int size, FILE *stream);
  int __kill(unsigned ins, int &error, pid_t pid, int sig);
  pid_t __fork(unsigned insid, int &error);
  int __execv(unsigned ins, int &error, const char *path, char *const argv[]);
  pid_t __wait(unsigned insid, int &error, int *status);
  pid_t __waitpid(unsigned insid, int &error, pid_t pid, int *status, int options);
  time_t __time(unsigned ins, int &error, time_t *t);
  int __clock_getres(unsigned ins, int &error, clockid_t clk_id, struct timespec *res);
  int __clock_gettime(unsigned ins, int &error, clockid_t clk_id, struct timespec *tp);
  int __clock_settime(unsigned ins, int &error, clockid_t clk_id, const struct timespec *tp);
  int __gettimeofday(unsigned ins, int &error, struct timeval *tv, struct timezone *tz);
  int __settimeofday(unsigned ins, int &error, const struct timeval *tv, const struct timezone *tz);

  // sleep
  int schedYield(unsigned ins, int &error);
  unsigned int __sleep(unsigned insid, int &error, unsigned int seconds);
  int __usleep(unsigned insid, int &error, useconds_t usec);
  int __nanosleep(unsigned insid, int &error, const struct timespec *req, struct timespec *rem);
  int __pthread_rwlock_rdlock(unsigned ins, int &error, pthread_rwlock_t *rwlock);
  int __pthread_rwlock_wrlock(unsigned ins, int &error, pthread_rwlock_t *rwlock);
  int __pthread_rwlock_tryrdlock(unsigned ins, int &error, pthread_rwlock_t *rwlock);
  int __pthread_rwlock_trywrlock(unsigned ins, int &error, pthread_rwlock_t *rwlock);
  int __pthread_rwlock_unlock(unsigned ins, int &error, pthread_rwlock_t *rwlock);
  int __pthread_rwlock_destroy(unsigned ins, int &error, pthread_rwlock_t *rwlock);
  int __pthread_rwlock_init(unsigned ins, int &error, pthread_rwlock_t *rwlock, const pthread_rwlockattr_t * attr);

  // print stat.
  void printStat();

  RecorderRT(): _Scheduler() {
    int ret;
    ret = sem_init(&thread_begin_sem, 0, 0);
    assert(!ret && "can't initialize semaphore!");
    ret = sem_init(&thread_begin_done_sem, 0, 0);
    assert(!ret && "can't initialize semaphore!");

    /// Schedule with paxos queue in the proxy process. 
    if (options::sched_with_paxos)
      paxq_open_shared_mem();
  }

  ~RecorderRT() {
  }

protected:

  /* These two sync wait/signal operations also contain logic for dbug+parrot, so name them separately.
  These two operations should only involve "sync" objects from applications or soft barrier hints. */
  int syncWait(void *chan, unsigned timeout = Scheduler::FOREVER);
  void syncSignal(void *chan, bool all=false);

  int absTimeToTurn(const struct timespec *abstime);
  int relTimeToTurn(const struct timespec *reltime);

  int pthreadMutexLockHelper(pthread_mutex_t *mutex, unsigned timeout = Scheduler::FOREVER);
  int pthreadRWLockWrLockHelper(pthread_rwlock_t *rwlock, unsigned timeout = Scheduler::FOREVER);
  int pthreadRWLockRdLockHelper(pthread_rwlock_t *rwlock, unsigned timeout = Scheduler::FOREVER);
  
  /// for each pthread barrier, track the count of the number and number
  /// of threads arrived at the barrier
  barrier_map barriers;

  /// for each opaque type, track the its ref counted barrier.
  refcnt_bar_map refcnt_bars;

  /// need these semaphores to assign tid deterministically; see comments
  /// for pthreadCreate() and threadBegin()
  sem_t thread_begin_sem;
  sem_t thread_begin_done_sem;


  /// Schedule with paxos queue in the proxy process.
  enum SyncType {
    DMT_REG_SYNC, /** regular pthreads sync, e.g., lock/unlock **/
    DMT_SELECT, /** select or poll or epoll_wait **/
    DMT_ACCEPT, /** accept or accept4 **/
    DMT_RECV /** recv or recvfrom or fread or read, etc **/
  };
  const char *charSyncType[4] = {"DMT_REG_SYNC", "DMT_SELECT", "DMT_ACCEPT", "DMT_RECV"};
  /** sockFd is only available for recv(), and for select() and accept() it is -1. **/
  paxos_op schedSocketOp(const char *funcName, SyncType syncType,
    long sockFd = -1, void *selectWaitObj = NULL, unsigned recvLen = 0);

  /// Stats.
  RuntimeStat stat;
};
} // namespace tern

#endif
