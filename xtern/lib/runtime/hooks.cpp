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

#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <poll.h>

#include "tern/config.h"
#include "tern/hooks.h"
#include "tern/space.h"
#include "tern/options.h"
#include "tern/runtime/runtime.h"
#include "tern/runtime/scheduler.h"
#include "helper.h"
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace tern;

// make this a weak symbol so that a runtime implementation can replace it
void __attribute((weak)) InstallRuntime() {
  assert(0&&"A Runtime must define its own InstallRuntime() to instal itself!");
}

#ifdef __cplusplus
extern "C" {
#endif

extern void *idle_thread(void*);

void tern_prog_begin() {
  assert(Space::isSys() && "tern_prog_begin must start in sys space");
  Runtime::the->progBegin();
  assert(Space::isSys() && "tern_prog_begin must end in sys space");
}

void tern_prog_end() {
  assert(Space::isSys() && "tern_prog_end must start in sys space");
  Runtime::the->progEnd();
  assert(Space::isSys() && "tern_prog_end must end in sys space");
}

void tern_symbolic_real(unsigned ins, void *addr,
                        int nbytes, const char *name) 
{
  int error = errno;
  Runtime::the->symbolic(ins, error, addr, nbytes, name);
  errno = error;
}

void tern_thread_begin(void) {
  assert(Space::isSys() && "tern_thread_begin must start in sys space");
  int error = errno;
  // thread begins in Sys space
  Runtime::the->threadBegin();
  Space::exitSys();
  errno = error;
  assert(Space::isApp() && "tern_thread_begin must end in app space");
}

extern void __prog_end_from_non_main_thread(void);

void tern_thread_end(unsigned ins) {
  assert(Space::isApp() && "tern_thread_end must start in app space");

  int error = errno;
  Space::enterSys();
  Runtime::the->threadEnd(ins);
  // thread ends in Sys space
  errno = error;
  assert(Space::isSys() && "tern_thread_end must end in sys space");
}

int tern_pthread_cancel(unsigned ins, pthread_t thread) {
  /* Fixme: a correct way of handling pthread_cancel() is: at the starting 
  point of each child thread, the child thread register a cleanup routine using 
  pthread_cleanup_push(), and then in this routine the thread removes itself 
  from runq. Currently the pthread_cancel mechaism in xtern ignores this, which 
  may cause a problem that a child thread is canceled and exits, but its thread 
  id is still in the runq.
  */
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadCancel(ins, error, thread);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_create(unsigned ins, pthread_t *thread,  const pthread_attr_t *attr,
                        void *(*thread_func)(void*), void *arg) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadCreate(ins, error, thread, const_cast<pthread_attr_t*>(attr),
                                           thread_func, arg);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_join(unsigned ins, pthread_t th, void **retval) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadJoin(ins, error, th, retval);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_detach(unsigned ins, pthread_t th) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__pthread_detach(ins, error, th);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_mutex_init(unsigned ins, pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
  int error = errno;
  int ret;
  Space::enterSys();
  if (options::set_mutex_errorcheck && mutexattr == NULL)
  {
//    pthread_mutexattr_t *sharedm = new pthread_mutexattr_t;
//    pthread_mutexattr_t &psharedm = *sharedm;
    pthread_mutexattr_t psharedm;
    pthread_mutexattr_init(&psharedm);
    pthread_mutexattr_settype(&psharedm, PTHREAD_MUTEX_ERRORCHECK);
    mutexattr = &psharedm;
  }
  //ret = pthread_mutex_init(mutex, mutexattr);
  //fprintf(stderr, "Hooks %s: pid %d, self %u start mutex %p, ins %p\n", __FUNCTION__, getpid(), (unsigned)pthread_self(), (void *)mutex, (void *)ins);
  ret = Runtime::the->pthreadMutexInit(ins, error, mutex, mutexattr);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_mutex_destroy(unsigned ins, pthread_mutex_t *mutex) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadMutexDestroy(ins, error, mutex);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_mutex_lock(unsigned ins, pthread_mutex_t *mutex) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadMutexLock(ins, error, mutex);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_mutex_trylock(unsigned ins, pthread_mutex_t *mutex) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadMutexTryLock(ins, error, mutex);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_mutex_timedlock(unsigned ins, pthread_mutex_t *mutex,
                                 const struct timespec *t) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadMutexTimedLock(ins, error, mutex, t);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_mutex_unlock(unsigned ins, pthread_mutex_t *mutex) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadMutexUnlock(ins, error, mutex);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_cond_wait(unsigned ins, pthread_cond_t *cv,pthread_mutex_t *mu){
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadCondWait(ins, error, cv, mu);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_cond_timedwait(unsigned ins, pthread_cond_t *cv,
                                pthread_mutex_t *mu, const struct timespec *t){
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadCondTimedWait(ins, error, cv, mu, t);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_cond_signal(unsigned ins, pthread_cond_t *cv) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadCondSignal(ins, error, cv);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_cond_broadcast(unsigned ins, pthread_cond_t *cv) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadCondBroadcast(ins, error, cv);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_barrier_init(unsigned ins, pthread_barrier_t *barrier,
                        const pthread_barrierattr_t * attr, unsigned count) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadBarrierInit(ins, error, barrier, count);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_barrier_wait(unsigned ins, pthread_barrier_t *barrier) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadBarrierWait(ins, error, barrier);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_barrier_destroy(unsigned ins, pthread_barrier_t *barrier) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->pthreadBarrierDestroy(ins, error, barrier);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_sem_init(unsigned ins, sem_t *sem, int pshared, unsigned int value) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->semInit(ins, error, sem, pshared, value);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_sem_wait(unsigned ins, sem_t *sem) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->semWait(ins, error, sem);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_sem_trywait(unsigned ins, sem_t *sem) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->semTryWait(ins, error, sem);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_sem_timedwait(unsigned ins, sem_t *sem, const struct timespec *t) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->semTimedWait(ins, error, sem, t);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_sem_post(unsigned ins, sem_t *sem) {
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->semPost(ins, error, sem);
  Space::exitSys();
  errno = error;
  return ret;
}

void tern_lineup_init_real(long opaque_type, unsigned count, unsigned timeout_turns) {
  int error = errno;
  Space::enterSys();
  Runtime::the->lineupInit(opaque_type, count, timeout_turns);
  Space::exitSys();
  errno = error;
}

void tern_lineup_destroy_real(long opaque_type) {
  int error = errno;
  Space::enterSys();
  Runtime::the->lineupDestroy(opaque_type);
  Space::exitSys();
  errno = error;
}

void tern_lineup_start_real(long opaque_type) {
  int error = errno;
  Space::enterSys();
  Runtime::the->lineupStart(opaque_type);
  Space::exitSys();
  errno = error;
}

void tern_lineup_end_real(long opaque_type) { 
  int error = errno;
  Space::enterSys();
  Runtime::the->lineupEnd(opaque_type);
  Space::exitSys();
  errno = error;
}

void tern_non_det_start_real() {
  int error = errno;
  Space::enterSys();
  Runtime::the->nonDetStart();
  Space::exitSys();
  errno = error;
}

void tern_non_det_end_real() {
  int error = errno;
  Space::enterSys();
  Runtime::the->nonDetEnd();
  Space::exitSys();
  errno = error;
}

void tern_detach_real() {
  int error = errno;
  Space::enterSys();
  Runtime::the->threadDetach();
  Space::exitSys();
  errno = error;
}

void tern_disable_sched_paxos_real() {
  int error = errno;
  Space::enterSys();
  Runtime::the->threadDisableSchedPaxos();
  Space::exitSys();
  errno = error;
}

void tern_set_base_time_real(struct timespec *ts) {
  int error = errno;
  Space::enterSys();
  Runtime::the->setBaseTime(ts);
  Space::exitSys();
  errno = error;
}


void tern_non_det_barrier_end_real(int bar_id, int cnt) {
  int error = errno;
  Space::enterSys();
  Runtime::the->nonDetBarrierEnd(bar_id, cnt);
  Space::exitSys();
  errno = error;
}

void tern_exit(unsigned ins, int status) {
  assert(0 && "why do we call tern_exit?");
  //  this will be called in __tern_prog_end after exit().
  tern_thread_end(ins); // main thread ends
  tern_prog_end();
  assert(Space::isSys());
  exit(status);
}

/// just a wrapper to tern_thread_end() and pthread_exit()
void tern_pthread_exit(unsigned ins, void *retval) {
  // don't call tern_thread_end() for the main thread, since we'll call
  // tern_prog_end() later (which calls tern_thread_end())
  if(Scheduler::self() != Scheduler::MainThreadTid)
  {
    tern_thread_end(ins);
  } else {
    // FIXME: Need to call exit() to stop the idle thread here because
    // when the main thread calls pthread_exit(), it may wait for all
    // current threads to finish, before calling __tern_prog_end().
    exit(0);
  }
  assert(Space::isSys());
  if (Scheduler::self() != Scheduler::IdleThreadTid)
    Runtime::__pthread_exit(retval);
  else
    pthread_exit(retval);
}

int tern_sigwait(unsigned ins, const sigset_t *set, int *sig)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__sigwait(ins, error, set, sig);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_socket(unsigned ins, int domain, int type, int protocol)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__socket(ins, error, domain, type, protocol);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_listen(unsigned ins, int sockfd, int backlog)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__listen(ins, error, sockfd, backlog);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_accept(unsigned ins, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__accept(ins, error, sockfd, cliaddr, addrlen);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_accept4(unsigned ins, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen, int flags)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__accept4(ins, error, sockfd, cliaddr, addrlen, flags);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_connect(unsigned ins, int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__connect(ins, error, sockfd, serv_addr, addrlen);
  Space::exitSys();
  errno = error;
  return ret;
}

struct hostent *tern_gethostbyname(unsigned ins, const char *name)
{
  int error = errno;
  struct hostent *ret;
  Space::enterSys();
  ret = Runtime::the->__gethostbyname(ins, error, name);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_gethostbyname_r(unsigned ins, const char *name, struct hostent *ret, char *buf, size_t buflen,
  struct hostent **result, int *h_errnop)
{
  int error = errno;
  int ret2;
  Space::enterSys();
  ret2 = Runtime::the->__gethostbyname_r(ins, error, name, ret, buf, buflen, result, h_errnop);
  Space::exitSys();
  errno = error;
  return ret2;
}

int tern_getaddrinfo(unsigned ins, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
  int error = errno;
  int ret2;
  Space::enterSys();
  ret2 = Runtime::the->__getaddrinfo(ins, error, node, service, hints, res);
  Space::exitSys();
  errno = error;
  return ret2;
}

void tern_freeaddrinfo(unsigned ins, addrinfo *res)
{
  int error = errno;
  Space::enterSys();
  Runtime::the->__freeaddrinfo(ins, error, res);
  Space::exitSys();
  errno = error;
}

struct hostent *tern_gethostbyaddr(unsigned ins, const void *addr, int len, int type)
{
  int error = errno;
  struct hostent *ret;
  Space::enterSys();
  ret = Runtime::the->__gethostbyaddr(ins, error, addr, len, type);
  Space::exitSys();
  errno = error;
  return ret;
}

char *tern_inet_ntoa(unsigned ins, struct in_addr in) {
  int error = errno;
  char *ret;
  Space::enterSys();
  ret = Runtime::the->__inet_ntoa(ins, error, in);
  Space::exitSys();
  errno = error;
  return ret;
}

char *tern_strtok(unsigned ins, char * str, const char * delimiters) {
  int error = errno;
  char *ret;
  Space::enterSys();
  ret = Runtime::the->__strtok(ins, error, str, delimiters);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_send(unsigned ins, int sockfd, const void *buf, size_t len, int flags)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__send(ins, error, sockfd, buf, len, flags);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_sendto(unsigned ins, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__sendto(ins, error, sockfd, buf, len, flags, dest_addr, addrlen);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_sendmsg(unsigned ins, int sockfd, const struct msghdr *msg, int flags)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__sendmsg(ins, error, sockfd, msg, flags);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_recv(unsigned ins, int sockfd, void *buf, size_t len, int flags)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__recv(ins, error, sockfd, buf, len, flags);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_recvfrom(unsigned ins, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__recvfrom(ins, error, sockfd, buf, len, flags, src_addr, addrlen);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_recvmsg(unsigned ins, int sockfd, struct msghdr *msg, int flags)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__recvmsg(ins, error, sockfd, msg, flags);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_shutdown(unsigned ins, int sockfd, int how)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__shutdown(ins, error, sockfd, how);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_getpeername(unsigned ins, int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__getpeername(ins, error, sockfd, addr, addrlen);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_getsockopt(unsigned ins, int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__getsockopt(ins, error, sockfd, level, optname, optval, optlen);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_setsockopt(unsigned ins, int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__setsockopt(ins, error, sockfd, level, optname, optval, optlen);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pipe(unsigned ins, int pipefd[2])
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__pipe(ins, error, pipefd);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_fcntl(unsigned ins, int fd, int cmd, void *arg)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__fcntl(ins, error, fd, cmd, arg);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_close(unsigned ins, int fd)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__close(ins, error, fd);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_read(unsigned ins, int fd, void *buf, size_t count)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__read(ins, error, fd, buf, count);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_write(unsigned ins, int fd, const void *buf, size_t count)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__write(ins, error, fd, buf, count);
  Space::exitSys();
  errno = error;
  return ret;
}

size_t tern_fread(unsigned ins, void * ptr, size_t size, size_t count, FILE * stream)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__fread(ins, error, ptr, size, count, stream);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_pread(unsigned ins, int fd, void *buf, size_t count, off_t offset)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__pread(ins, error, fd, buf, count, offset);
  Space::exitSys();
  errno = error;
  return ret;
}

ssize_t tern_pwrite(unsigned ins, int fd, const void *buf, size_t count, off_t offset)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__pwrite(ins, error, fd, buf, count, offset);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_epoll_wait(unsigned ins, int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__epoll_wait(ins, error, epfd, events, maxevents, timeout);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_epoll_create(unsigned ins, int size)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__epoll_create(ins, error, size);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_epoll_ctl(unsigned ins, int epfd, int op, int fd, struct epoll_event *event)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__epoll_ctl(ins, error, epfd, op, fd, event);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_select(unsigned ins, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__select(ins, error, nfds, readfds, writefds, exceptfds, timeout);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_poll(unsigned ins, struct pollfd *fds, nfds_t nfds, int timeout)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__poll(ins, error, fds, nfds, timeout);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_bind(unsigned ins, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__bind(ins, error, sockfd, addr, addrlen);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_sched_yield(unsigned ins)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->schedYield(ins, error);
  Space::exitSys();
  errno = error;
  return ret;
}

unsigned int tern_sleep(unsigned ins, unsigned int seconds)
{
  int error = errno;
  unsigned int ret;
  Space::enterSys();
  ret = Runtime::the->__sleep(ins, error, seconds);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_usleep(unsigned ins, useconds_t usec)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__usleep(ins, error, usec);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_nanosleep(unsigned ins, const struct timespec *req, struct timespec *rem)
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__nanosleep(ins, error, req, rem);
  Space::exitSys();
  errno = error;
  return ret;
}

char *tern_fgets(unsigned ins, char *s, int size, FILE *stream)
{
  int error = errno;
  char *ret;
  Space::enterSys();
  ret = Runtime::the->__fgets(ins, error, s, size, stream);
  Space::exitSys();
  errno = error;
  return ret;
}

void tern_idle_sleep()
{
  Space::enterSys();
  Runtime::the->idle_sleep();
  Space::exitSys();
}

void tern_idle_cond_wait()
{
  Space::enterSys();
  Runtime::the->idle_cond_wait();
  Space::exitSys();
}

int tern_kill(unsigned ins, pid_t pid, int sig)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__kill(ins, error, pid, sig);
  Space::exitSys();
  errno = error;
  return ret;
}

pid_t tern_fork(unsigned ins)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__fork(ins, error);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_execv(unsigned ins, const char *path, char *const argv[])
{
  int error = errno;
  int ret;
  Space::enterSys();
  ret = Runtime::the->__execv(ins, error, path, argv);
  Space::exitSys();
  errno = error;
  return ret;
}

pid_t tern_wait(unsigned ins, int *status)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__wait(ins, error, status);
  Space::exitSys();
  errno = error;
  return ret;
}

pid_t tern_waitpid(unsigned ins, pid_t pid, int *status, int options)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__waitpid(ins, error, pid, status, options);
  Space::exitSys();
  errno = error;
  return ret;
}

time_t tern_time(unsigned ins, time_t *t)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__time(ins, error, t);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_clock_getres(unsigned ins, clockid_t clk_id, struct timespec *res)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__clock_getres(ins, error, clk_id, res);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_clock_gettime(unsigned ins, clockid_t clk_id, struct timespec *tp)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__clock_gettime(ins, error, clk_id, tp);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_clock_settime(unsigned ins, clockid_t clk_id, const struct timespec *tp)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__clock_settime(ins, error, clk_id, tp);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_gettimeofday(unsigned ins, struct timeval *tv, struct timezone *tz)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__gettimeofday(ins, error, tv, tz);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_settimeofday(unsigned ins, const struct timeval *tv, const struct timezone *tz)
{
  int error = errno;
  pid_t ret;
  Space::enterSys();
  ret = Runtime::the->__settimeofday(ins, error, tv, tz);
  Space::exitSys();
  errno = error;
  return ret;
}

int tern_pthread_rwlock_rdlock(unsigned ins, pthread_rwlock_t *rwlock) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_rdlock(ins, error, rwlock); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

int tern_pthread_rwlock_tryrdlock(unsigned ins, pthread_rwlock_t *rwlock) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_tryrdlock(ins, error, rwlock); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

int tern_pthread_rwlock_wrlock(unsigned ins, pthread_rwlock_t *rwlock) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_wrlock(ins, error, rwlock); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

int tern_pthread_rwlock_trywrlock(unsigned ins, pthread_rwlock_t *rwlock) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_trywrlock(ins, error, rwlock); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

int tern_pthread_rwlock_unlock(unsigned ins, pthread_rwlock_t *rwlock) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_unlock(ins, error, rwlock); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

int tern_pthread_rwlock_destroy(unsigned ins, pthread_rwlock_t *rwlock) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_destroy(ins, error, rwlock); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

int tern_pthread_rwlock_init(unsigned ins, pthread_rwlock_t *rwlock, const pthread_rwlockattr_t * attr) 
{ 
  int error = errno; 
  int ret; 
  Space::enterSys(); 
  ret = Runtime::the->__pthread_rwlock_init(ins, error, rwlock, attr); 
  Space::exitSys(); 
  errno = error; 
  return ret; 
}

void tern_print_runtime_stat()
{
  Space::enterSys();
  Runtime::the->printStat();
  Space::exitSys();
}

/*
int tern_poll(unsigned ins, struct pollfd *fds, nfds_t nfds, int timeout)
{
  errno = error;
  return poll(fds, nfds_t nfds, timeout);
}

int tern_open(unsigned ins, const char *pathname, int flags)
{
  errno = error;
  return open(pathname, flags);
}

int tern_open(unsigned ins, const char *pathname, int flags, mode_t mode)
{
  errno = error;
  return open(pathname, flags,mode);
}

int tern_creat(unsigned ins, const char *pathname, mode_t mode)
{
  errno = error;
  return creat(pathname,mode);
}

int tern_ppoll(unsigned ins, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
{
  errno = error;
  return ppoll(fds, nfds_t nfds, timeout_ts, sigmask);
}

int tern_pselect(unsigned ins, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask)
{
  errno = error;
  return pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
}

void tern_FD_CLR(unsigned ins, int fd, fd_set *set)
{
  errno = error;
  return FD_CLR(fd, set);
}

int tern_FD_ISSET(unsigned ins, int fd, fd_set *set)
{
  errno = error;
  return FD_ISSET(fd, set);
}

void tern_FD_SET(unsigned ins, int fd, fd_set *set)
{
  errno = error;
  return FD_SET(fd, set);
}

void tern_FD_ZERO(unsigned ins, fd_set *set)
{
  errno = error;
  return FD_ZERO(set);
}

int tern_sockatmark (unsigned ins, int fd)
{
  errno = error;
  return sockatmark (fd);
}

int tern_isfdtype(unsigned ins, int fd, int fdtype)
{
  errno = error;
  return isfdtype(fd, fdtype);
}

*/

#ifdef __cplusplus
}
#endif


