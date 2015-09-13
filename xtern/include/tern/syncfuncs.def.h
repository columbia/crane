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

/* This is the central table that lists the synchronization functions and
 * blocking system calls that tern hooks, as well as tern builtin synch
 * functions. */

/* intentionally no include guard */

/* format:
 *   DEF(func, kind, ret_type, arg0, arg1, ...)
 *   DEFTERN(func)  for tern builtin sync events
 */

/* pthread synchronization operations */
DEF(pthread_create,         Synchronization, int, pthread_t *thread, const pthread_attr_t *attr, void* (*start_routine)(void *), void *arg)
DEF(pthread_join,           Synchronization, int, pthread_t th, void **thread_return)
DEF(pthread_detach,           Synchronization, int, pthread_t th)
DEF(pthread_cancel,         Synchronization, int, pthread_t thread)
DEF(pthread_exit,           Synchronization, void, void *retval)
DEF(pthread_mutex_init,     Synchronization, int, pthread_mutex_t *mutex, const  pthread_mutexattr_t *mutexattr)
DEF(pthread_mutex_lock,     Synchronization, int, pthread_mutex_t *mutex)
DEF(pthread_mutex_unlock,   Synchronization, int, pthread_mutex_t *mutex)
DEF(pthread_mutex_trylock,  Synchronization, int, pthread_mutex_t *mutex)
DEF(pthread_mutex_timedlock,Synchronization, int, pthread_mutex_t *mutex, const struct timespec *abstime)
DEF(pthread_mutex_destroy,  Synchronization, int, pthread_mutex_t *mutex)

/*  rwlock */
DEF(pthread_rwlock_rdlock, Synchronization, int, pthread_rwlock_t *rwlock)
DEF(pthread_rwlock_tryrdlock, Synchronization, int, pthread_rwlock_t *rwlock)
DEF(pthread_rwlock_trywrlock, Synchronization, int, pthread_rwlock_t *rwlock)
DEF(pthread_rwlock_wrlock, Synchronization, int, pthread_rwlock_t *rwlock)
DEF(pthread_rwlock_unlock, Synchronization, int, pthread_rwlock_t *rwlock)
DEF(pthread_rwlock_destroy, Synchronization, int, pthread_rwlock_t *rwlock)
DEF(pthread_rwlock_init, Synchronization, int, pthread_rwlock_t * rwlock, const pthread_rwlockattr_t * attr) 
//DEF(pthread_rwlock_timedrdlock, Synchronization, int, pthread_rwlock_t * rwlock, const struct timespec * abs_timeout)
//DEF(pthread_rwlock_timedwrlock, Synchronization, int, pthread_rwlock_t * rwlock, const struct timespec * abs_timeout)


// DEF(pthread_cond_init,      Synchronization, int, pthread_cond_t *cond, pthread_condattr_t*attr)
DEF(pthread_cond_wait,      Synchronization, int, pthread_cond_t *cond, pthread_mutex_t *mutex)
DEF(pthread_cond_timedwait, Synchronization, int, pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
DEF(pthread_cond_broadcast, Synchronization, int, pthread_cond_t *cond)
DEF(pthread_cond_signal,    Synchronization, int, pthread_cond_t *cond)
// DEF(pthread_cond_destroy,   Synchronization, int, pthread_cond_t *cond)
DEF(pthread_barrier_init,   Synchronization, int, pthread_barrier_t *barrier, const pthread_barrierattr_t* attr, unsigned count)
DEF(pthread_barrier_wait,   Synchronization, int, pthread_barrier_t *barrier)
DEF(pthread_barrier_destroy,Synchronization, int, pthread_barrier_t *barrier)
DEF(sem_init,               Synchronization, int, sem_t *sem, int pshared, unsigned int value)
DEF(sem_post,               Synchronization, int, sem_t *sem)
DEF(sem_wait,               Synchronization, int, sem_t *sem)
DEF(sem_trywait,            Synchronization, int, sem_t *sem)
DEF(sem_timedwait,          Synchronization, int, sem_t *sem, const struct timespec *abs_timeout)
// DEF(sem_destroy,            Synchronization, int, sem_t *sem)

/* socket functions and file functions */
//	blockings: accept, connect, recv, recvfrom, recvmsg, read, select 
DEF(socket, Synchronization, int, int domain, int type, int protocol)
DEF(listen, Synchronization, int, int sockfd, int backlog)
DEF(accept, BlockingSyscall, int, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen)
DEF(accept4, BlockingSyscall, int, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen, int flags)
DEF(connect, BlockingSyscall, int, int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
DEF(gethostbyname, Synchronization, struct hostent*, const char *name)
DEF(gethostbyname_r, Synchronization, int, const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop)
DEF(getaddrinfo, Synchronization, int, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
DEF(freeaddrinfo, Synchronization, void, struct addrinfo *res)
DEF(gethostbyaddr, Synchronization, struct hostent*, const void *addr, int len, int type)
DEF(inet_ntoa, Synchronization, char *, struct in_addr in)
DEF(strtok, Synchronization, char *, char * str, const char * delimiters)
DEF(send, Synchronization, ssize_t, int sockfd, const void *buf, size_t len, int flags)
DEF(sendto, Synchronization, ssize_t, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
DEF(sendmsg, Synchronization, ssize_t, int sockfd, const struct msghdr *msg, int flags)
DEF(recv, BlockingSyscall, ssize_t, int sockfd, void *buf, size_t len, int flags)
DEF(recvfrom, BlockingSyscall, ssize_t, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
DEF(recvmsg, BlockingSyscall, ssize_t, int sockfd, struct msghdr *msg, int flags)
DEF(shutdown, Synchronization, int, int sockfd, int how)
DEF(getpeername, Synchronization, int, int sockfd, struct sockaddr *addr, socklen_t *addrlen)  
DEF(getsockopt, Synchronization, int, int sockfd, int level, int optname, void *optval, socklen_t *optlen)
DEF(setsockopt, Synchronization, int, int sockfd, int level, int optname, const void *optval, socklen_t optlen)
DEF(pipe, BlockingSyscall, int, int pipefd[2])
DEF(fcntl, Synchronization, int, int fd, int cmd, void *arg)

DEF(close, Synchronization, int, int fd)
DEF(read, BlockingSyscall, ssize_t, int fd, void *buf, size_t count)
DEF(write, Synchronization, ssize_t, int fd, const void *buf, size_t count)
DEF(fread, BlockingSyscall, size_t, void * ptr, size_t size, size_t count, FILE * stream)
DEF(pread, BlockingSyscall, ssize_t, int fd, void *buf, size_t count, off_t offset)
DEF(pwrite, Synchronization, ssize_t, int fd, const void *buf, size_t count, off_t offset)
DEF(select, BlockingSyscall, int, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
DEF(poll, Synchronization, int, struct pollfd *fds, nfds_t nfds, int timeout)
DEF(bind, Synchronization, int, int socket, const struct sockaddr *address, socklen_t address_len)
DEF(kill, Synchronization, int, pid_t pid, int sig)
DEF(fork, Synchronization, pid_t)
DEF(execv, Synchronization, int, const char *path, char *const argv[])
DEF(sched_yield, Synchronization, int)
DEF(wait, Synchronization, pid_t, int *status)
DEF(waitpid, Synchronization, pid_t, pid_t pid, int *status, int options)

//  real time functions
DEF(time, Synchronization, time_t, time_t *t)
DEF(clock_getres, Synchronization, int, clockid_t clk_id, struct timespec *res)
DEF(clock_gettime, Synchronization, int, clockid_t clk_id, struct timespec *tp)
DEF(clock_settime, Synchronization, int, clockid_t clk_id, const struct timespec *tp)
DEF(gettimeofday, Synchronization, int, struct timeval *tv, struct timezone *tz)
DEF(settimeofday, Synchronization, int, const struct timeval *tv, const struct timezone *tz)

// file operations not handled not.
//DEF(open, Synchronization, int, const char *pathname, int flags)
//DEF(open, Synchronization, int, const char *pathname, int flags, mode_t mode)
//DEF(creat, Synchronization, int, const char *pathname, mode_t mode)
//DEF(ppoll, Synchronization, int, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
//DEF(pselect, Synchronization, int, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask)
//DEF(FD_CLR, Synchronization, void, int fd, fd_set *set)
//DEF(FD_ISSET, Synchronization, int, int fd, fd_set *set)
//DEF(FD_SET, Synchronization, void, int fd, fd_set *set)
//DEF(FD_ZERO, Synchronization, void, fd_set *set)

// seems like apis in different arch.
//DEF(sockatmark, Synchronization, int, int fd)
//DEF(isfdtype, Synchronization, int, int fd, int fdtype)

/* blocking system calls */
DEF(sleep,                  BlockingSyscall, unsigned int, unsigned int seconds)
DEF(usleep,                 BlockingSyscall, int, useconds_t usec)
DEF(nanosleep,              BlockingSyscall, int, const struct timespec *req, struct timespec *rem)
//socket DEF(accept,                 BlockingSyscall, int, int sockfd, struct sockaddr *addr, socklen_t *addrlen)
//socket DEF(select,                 BlockingSyscall, int, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
DEF(epoll_wait,             BlockingSyscall, int, int epfd, struct epoll_event *events, int maxevents, int timeout)
DEF(epoll_create,             Synchronization, int, int size)
DEF(epoll_ctl, Synchronization, int, int epfd, int op, int fd, struct epoll_event *event)
DEF(sigwait,                BlockingSyscall, int, const sigset_t *set, int *sig)
DEF(fgets,                  BlockingSyscall, char*, char* s, int size, FILE *stream)
/* should include sched_yield */

/* We don't consider exit a sync event because our instrumentation code
 * uses other ways to intercept the end of program event.  Specifically,
 * our static instrumentation code uses the static constructor/destructor,
 * and our dynamic instrumentation code intercepts the init and fini
 * functions used by libc. */
/* DEF(exit,                   BlockingSyscall, void, int status) */
/* DEF(syscall,                BlockingSyscall, tern_, int, int) */ /* FIXME: why include generic syscall entry point? */
/* DEF(ap_mpm_pod_check,       BlockingSyscall, tern_) */ /* FIXME: ap_mpm_pod_check is not a real lib call; needed for apache */

DEFTERNUSER(tern_symbolic)
DEFTERNAUTO(tern_thread_begin)
DEFTERNAUTO(tern_thread_end)
DEFTERNUSER(tern_task_begin)
DEFTERNUSER(tern_task_end)
DEFTERNUSER(tern_lineup_init)
DEFTERNUSER(tern_lineup_destroy)
DEFTERNUSER(tern_lineup_start)
DEFTERNUSER(tern_lineup_end)
DEFTERNUSER(tern_lineup)
DEFTERNUSER(tern_non_det_start)
DEFTERNUSER(tern_non_det_end)
DEFTERNAUTO(tern_fix_up)
DEFTERNAUTO(tern_fix_down)
DEFTERNAUTO(tern_idle)

