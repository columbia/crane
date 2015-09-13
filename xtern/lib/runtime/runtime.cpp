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
#include <fcntl.h>
#include <string>
#include <poll.h>
#include <stdarg.h>

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
#include <sys/types.h>
#include <sys/wait.h>

//#define DEBUG_RUNTIME

#ifdef DEBUG_RUNTIME
#define dprintf(fmt...) fprintf(stderr, fmt);
#else
#define dprintf(fmt...)
#endif

using namespace tern;

Runtime *Runtime::the = NULL;

int __thread TidMap::self_tid  = -1;

extern pthread_t idle_th;

static bool sock_nonblock (int fd)
{
#ifndef __WIN32__
  return fcntl (fd, F_SETFL, O_NONBLOCK) >= 0;
#else
  u_long a = 1;
  return ioctlsocket (fd, FIONBIO, &a) >= 0;
#endif
}

Runtime::Runtime() {
}

int Runtime::pthreadCancel(unsigned insid, int &error, pthread_t thread)
{
  errno = error;
  int ret = pthread_cancel(thread);
  error = errno;
  return ret;
}

int Runtime::pthreadMutexInit(unsigned insid, int &error, pthread_mutex_t *mutex, const  pthread_mutexattr_t *mutexattr)
{
  errno = error;
  int ret = pthread_mutex_init(mutex, mutexattr);
  error = errno;
  return ret;
}

int Runtime::pthreadMutexDestroy(unsigned insid, int &error, pthread_mutex_t *mutex)
{
  errno = error;
  int ret = pthread_mutex_destroy(mutex);
  error = errno;
  return ret;
}

int Runtime::__pthread_create(pthread_t *th, const pthread_attr_t *a, void *(*func)(void*), void *arg) {
  int ret;
  ret = pthread_create(th, a, func, arg);
  return ret;
}

void Runtime::__pthread_exit(void *value_ptr) {
  pthread_exit(value_ptr);
}

int Runtime::__pthread_join(pthread_t th, void **retval) {
  return pthread_join(th, retval);
}

int Runtime::__pthread_detach(unsigned ins, int &error, pthread_t th) {
  int ret;
  errno = error;
  ret = pthread_detach(th);
  errno = error;
  return ret;
}

int Runtime::__socket(unsigned ins, int &error, int domain, int type, int protocol)
{
  errno = error;
  int ret;
  ret = socket(domain, type, protocol);
  error = errno;
  return ret;
}

int Runtime::__getsockopt(unsigned ins, int &error, int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
  errno = error;
  int ret;
  ret = ::getsockopt(sockfd, level, optname, optval, optlen);
  error = errno;
  return ret;
}

int Runtime::__setsockopt(unsigned ins, int &error, int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
  errno = error;
  int ret;
  ret = ::setsockopt(sockfd, level, optname, optval, optlen);
  error = errno;
  return ret;
}

int Runtime::__pipe(unsigned ins, int &error, int pipefd[2])
{
  errno = error;
  int ret;
  ret = ::pipe(pipefd);
  error = errno;
  return ret;
}

int Runtime::__fcntl(unsigned ins, int &error, int fd, int cmd, void *arg)
{
  errno = error;
  int ret;
  ret = ::fcntl(fd, cmd, arg);
  error = errno;
  return ret;
}

int Runtime::__listen(unsigned ins, int &error, int sockfd, int backlog)
{
  errno = error;
  int ret;
  ret = listen(sockfd, backlog);
  error = errno;
  return ret;
}

int Runtime::__accept(unsigned ins, int &error, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen)
{
  errno = error;
  int ret;
  ret = accept(sockfd, cliaddr, addrlen);
  if (options::non_block_recv)
    assert(sock_nonblock(sockfd));
  error = errno;
  return ret;
}

int Runtime::__accept4(unsigned ins, int &error, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen, int flags)
{
  errno = error;
  int ret;
  ret = accept4(sockfd, cliaddr, addrlen, flags);
  error = errno;
  return ret;
}

int Runtime::__connect(unsigned ins, int &error, int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
  errno = error;
  int ret;
  ret = connect(sockfd, serv_addr, addrlen);
  if (options::non_block_recv)
    assert(sock_nonblock(sockfd));
  error = errno;
  return ret;
}

struct hostent *Runtime::__gethostbyname(unsigned ins, int &error, const char *name)
{
  errno = error;
  struct hostent *ret;
  ret = gethostbyname(name);
  error = errno;
  return ret;
}

int Runtime::__gethostbyname_r(unsigned ins, int &error, const char *name,
  struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
  errno = error;
  int ret2;
  ret2 = gethostbyname_r(name, ret, buf, buflen, result, h_errnop);
  error = errno;
  return ret2;
}

int Runtime::__getaddrinfo(unsigned insid, int &error, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
  errno = error;
  int ret;
  ret = getaddrinfo(node, service, hints, res);
  error = errno;
  return ret;
}

void  Runtime::__freeaddrinfo(unsigned insid, int &error, struct addrinfo *res) {
  errno = error;
  freeaddrinfo(res);
  error = errno;
}

struct hostent *Runtime::__gethostbyaddr(unsigned ins, int &error, const void *addr, int len, int type)
{
  errno = error;
  struct hostent *ret;
  ret = gethostbyaddr(addr, len, type);
  error = errno;
  return ret;
}

char *Runtime::__inet_ntoa(unsigned ins, int &error, struct in_addr in)
{
  errno = error;
  char *ret;
  ret = inet_ntoa(in);
  error = errno;
  return ret;
}

char *Runtime::__strtok(unsigned ins, int &error, char * str, const char * delimiters)
{
  errno = error;
  char *ret;
  ret = strtok(str, delimiters);
  error = errno;
  return ret;
}

ssize_t Runtime::__send(unsigned ins, int &error, int sockfd, const void *buf, size_t len, int flags)
{
  errno = error;
  ssize_t ret;
  ret = send(sockfd, buf, len, flags);
  error = errno;
  return ret;
}

ssize_t Runtime::__sendto(unsigned ins, int &error, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
  errno = error;
  ssize_t ret;
  ret = sendto(sockfd,buf,len,flags,dest_addr,addrlen);
  error = errno;
  return ret;
}

ssize_t Runtime::__sendmsg(unsigned ins, int &error, int sockfd, const struct msghdr *msg, int flags)
{
  errno = error;
  ssize_t ret;
  ret = sendmsg(sockfd,msg,flags);
  error = errno;
  return ret;
}

ssize_t Runtime::__recv(unsigned ins, int &error, int sockfd, void *buf, size_t len, int flags)
{
  errno = error;
  ssize_t ret;
  ret = 0;
  int try_count = 0;
  timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000 * 1000 * 100; // 10 ms;
  while ((int) ret < (int) len && try_count < 10)
  {
    ssize_t sr = recv(sockfd, (char*)buf + ret, len - ret, flags);

    if (sr >= 0) 
      ret += sr;
    else if (ret == 0)
      ret = -1;

    if (sr < 0 || !options::non_block_recv) // it's the end of a package
      break;

    //fprintf(stderr, "sr = %d\n", (int) sr);

    if (!sr) {
      ::nanosleep(&ts, NULL);
      ++try_count;
    }
  }
  error = errno;
  return ret;
}

ssize_t Runtime::__recvfrom(unsigned ins, int &error, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
  errno = error;
  ssize_t ret;
  ret = recvfrom(sockfd,buf,len,flags,src_addr,addrlen);
  error = errno;
  return ret;
}

ssize_t Runtime::__recvmsg(unsigned ins, int &error, int sockfd, struct msghdr *msg, int flags)
{
  errno = error;
  ssize_t ret;
  ret = recvmsg(sockfd,msg,flags);
  error = errno;
  return ret;
}

int Runtime::__shutdown(unsigned ins, int &error, int sockfd, int how)
{
  errno = error;
  ssize_t ret;
  ret = ::shutdown(sockfd,how);
  error = errno;
  return ret;
}

int Runtime::__getpeername(unsigned ins, int &error, int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  errno = error;
  int ret = getpeername(sockfd, addr, addrlen);
  error = errno;
  return ret;
}

int Runtime::__close(unsigned ins, int &error, int fd)
{
  errno = error;
  int ret;
  ret = close(fd);
  error = errno;
  return ret;
}

ssize_t Runtime::__read(unsigned ins, int &error, int fd, void *buf, size_t count)
{
  errno = error;
  ssize_t ret;
  ret = read(fd, buf, count);
  error = errno;
  return ret;
}

ssize_t Runtime::__write(unsigned ins, int &error, int fd, const void *buf, size_t count)
{
  errno = error;
  ssize_t ret;
  ret = write(fd, buf, count);
  error = errno;
  return ret;
}

size_t Runtime::__fread(unsigned ins, int &error, void * ptr, size_t size, size_t count, FILE * stream)
{
  errno = error;
  ssize_t ret;
  ret = fread(ptr, size, count, stream);
  error = errno;
  return ret;
}

ssize_t Runtime::__pread(unsigned ins, int &error, int fd, void *buf, size_t count, off_t offset)
{
  errno = error;
  ssize_t ret;
  ret = pread(fd, buf, count, offset);
  error = errno;
  return ret;
}

ssize_t Runtime::__pwrite(unsigned ins, int &error, int fd, const void *buf, size_t count, off_t offset)
{
  errno = error;
  ssize_t ret;
  ret = pwrite(fd, buf, count, offset);
  error = errno;
  return ret;
}

int Runtime::__select(unsigned ins, int &error, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
  errno = error;
  int ret;
  ret = select(nfds, readfds, writefds, exceptfds, timeout);
  error = errno;
  return ret;
}

int Runtime::__poll(unsigned ins, int &error, struct pollfd *fds, nfds_t nfds, int timeout)
{
  errno = error;
  int ret;
  ret = poll(fds, nfds, timeout);
  error = errno;
  return ret;
}

int Runtime::__bind(unsigned ins, int &error, int socket, const struct sockaddr *address, socklen_t address_len)
{
  errno = error;
  int ret;
  ret = bind(socket, address, address_len);
  error = errno;
  return ret;
}

int Runtime::__sigwait(unsigned ins, int &error, const sigset_t *set, int *sig)
{
  errno = error;
  int ret;
  ret = sigwait(set, sig);
  error = errno;
  return ret;
}

int Runtime::__epoll_wait(unsigned ins, int &error, int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  errno = error;
  int ret;
  ret = ::epoll_wait(epfd,events,maxevents,timeout);
  error = errno;
  return ret;
}

int Runtime::__epoll_create(unsigned ins, int &error, int size)
{
  errno = error;
  int ret;
  ret = ::epoll_create(size);
  error = errno;
  return ret;
}

int Runtime::__epoll_ctl(unsigned ins, int &error, int epfd, int op, int fd, struct epoll_event *event)
{
  errno = error;
  int ret;
  ret = ::epoll_ctl(epfd,op,fd,event);
  error = errno;
  return ret;
}

unsigned int Runtime::__sleep(unsigned ins, int &error, unsigned int seconds)
{
  errno = error;
  unsigned int ret;
  ret = ::sleep(seconds);
  error = errno;
  return ret;
}

int Runtime::__usleep(unsigned ins, int &error, useconds_t usec)
{
  errno = error;
  int ret;
  ret = ::usleep(usec);
  error = errno;
  return ret;
}

char *Runtime::__fgets(unsigned ins, int &error, char *s, int size, FILE *stream)
{
  errno = error;
  char* ret;
  ret = fgets(s, size, stream);
  error = errno;
  return ret;
}

void Runtime::___exit(int status) {
  _exit(status);
}

pid_t Runtime::__kill(unsigned ins, int &error, pid_t pid, int sig)
{
  errno = error;
  pid_t ret;
  ret = ::kill(pid, sig);
  error = errno;
  return ret;
}

pid_t Runtime::__fork(unsigned ins, int &error)
{
  errno = error;
  pid_t ret;
  ret = fork();
  error = errno;
  return ret;
}

int Runtime::__execv(unsigned ins, int &error, const char *path, char *const argv[]) {
  errno = error;
  int ret;
  ret = ::execv(path, argv);
  error = errno;
  return ret;
}

pid_t Runtime::__wait(unsigned ins, int &error, int *status)
{
  errno = error;
  pid_t ret;
  ret = wait(status);
  error = errno;
  return ret;
}

pid_t Runtime::__waitpid(unsigned ins, int &error, pid_t pid, int *status, int options)
{
  errno = error;
  pid_t ret;
  ret = waitpid(pid, status, options);
  error = errno;
  return ret;
}

int Runtime::__sched_yield(unsigned ins, int &error) {
  errno = error;
  pid_t ret;
  ret = sched_yield();
  error = errno;
  return ret;
}

int Runtime::__nanosleep(unsigned ins, int &error, const struct timespec *req, struct timespec *rem)
{
  errno = error;
  int ret;
  ret = ::nanosleep(req, rem);
  error = errno;
  return ret;
}

time_t Runtime::__time(unsigned ins, int &error, time_t *t)
{
  errno = error;
  time_t ret = ::time(t);
  error = errno;
  return ret;
}

int Runtime::__clock_getres(unsigned ins, int &error, clockid_t clk_id, struct timespec *res)
{
  errno = error;
  int ret = ::clock_getres(clk_id, res);
  error = errno;
  return ret;
}

int Runtime::__clock_gettime(unsigned ins, int &error, clockid_t clk_id, struct timespec *tp)
{
  errno = error;
  int ret = ::clock_gettime(clk_id, tp);
  error = errno;
  return ret;
}

int Runtime::__clock_settime(unsigned ins, int &error, clockid_t clk_id, const struct timespec *tp)
{
  errno = error;
  int ret = ::clock_settime(clk_id, tp);
  error = errno;
  return ret;
}

int Runtime::__gettimeofday(unsigned ins, int &error, struct timeval *tv, struct timezone *tz)
{
  errno = error;
  int ret = ::gettimeofday(tv, tz);
  error = errno;
  return ret;
}

int Runtime::__settimeofday(unsigned ins, int &error, const struct timeval *tv, const struct timezone *tz)
{
  errno = error;
  int ret = ::settimeofday(tv, tz);
  error = errno;
  return ret;
}

int Runtime::__sem_init(unsigned insid, int &error, sem_t *sem, int pshared, unsigned int value) {
  errno = error;
  int ret;
  ret = sem_init(sem, pshared, value);
  error = errno;
  return ret;
}

int Runtime::__sem_wait(unsigned insid, int &error, sem_t *sem) {
  errno = error;
  int ret;
  ret = sem_wait(sem);
  error = errno;
  return ret;
}

int Runtime::__sem_post(unsigned insid, int &error, sem_t *sem) {
  errno = error;
  int ret;
  ret = sem_post(sem);
  error = errno;
  return ret;
}

int Runtime::__pthread_mutex_init(unsigned insid, int &error, pthread_mutex_t *mutex, const  pthread_mutexattr_t *mutexattr) {
  errno = error;
  int ret;
  ret = pthread_mutex_init(mutex, mutexattr);
  error = errno;
  return ret;
}

int Runtime::__pthread_mutex_destroy(unsigned insid, int &error, pthread_mutex_t *mutex) {
  errno = error;
  int ret;
  ret = pthread_mutex_destroy(mutex);
  error = errno;
  return ret;
}

int Runtime::__pthread_mutex_lock(unsigned insid, int &error, pthread_mutex_t *mutex) {
  errno = error;
  int ret;
  ret = pthread_mutex_lock(mutex);
  error = errno;
  return ret;
}

int Runtime::__pthread_mutex_unlock(unsigned insid, int &error, pthread_mutex_t *mutex) {
  errno = error;
  int ret;
  ret = pthread_mutex_unlock(mutex);
  error = errno;
  return ret;
}

int Runtime::__pthread_rwlock_rdlock(unsigned ins, int &error, pthread_rwlock_t *rwlock) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_rdlock(rwlock); 
  errno = error; 
  return ret; 
}

int Runtime::__pthread_rwlock_tryrdlock(unsigned ins, int &error, pthread_rwlock_t *rwlock) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_tryrdlock(rwlock); 
  errno = error; 
  return ret; 
}

int Runtime::__pthread_rwlock_wrlock(unsigned ins, int &error, pthread_rwlock_t *rwlock) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_wrlock(rwlock); 
  errno = error; 
  return ret; 
}

int Runtime::__pthread_rwlock_trywrlock(unsigned ins, int &error, pthread_rwlock_t *rwlock) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_trywrlock(rwlock); 
  errno = error; 
  return ret; 
}

int Runtime::__pthread_rwlock_unlock(unsigned ins, int &error, pthread_rwlock_t *rwlock) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_unlock(rwlock); 
  errno = error; 
  return ret; 
}

int Runtime::__pthread_rwlock_destroy(unsigned ins, int &error, pthread_rwlock_t *rwlock) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_destroy(rwlock); 
  errno = error; 
  return ret; 
}

int Runtime::__pthread_rwlock_init(unsigned ins, int &error, pthread_rwlock_t *rwlock, const pthread_rwlockattr_t * attr) 
{ 
  error = errno; 
  int ret = ::pthread_rwlock_init(rwlock, attr); 
  errno = error; 
  return ret; 
}


