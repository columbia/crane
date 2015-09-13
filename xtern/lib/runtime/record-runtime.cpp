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

// Authors: Heming Cui (heming@cs.columbia.edu), Junfeng Yang (junfeng@cs.columbia.edu).  Refactored from
// Heming's Memoizer code
//
// Some tricky issues addressed or discussed here are:
//
// 1. deterministic pthread_create
// 2. deterministic and deadlock-free pthread_barrier_wait
// 3. deterministic and deadlock-free pthread_cond_wait
// 4. timed wait operations (e.g., pthread_cond_timewait)
// 5. try-operations (e.g., pthread_mutex_trylock)
//
// TODO:
// 1. implement the other proposed solutions to pthread_cond_wait
// 2. design and implement a physical-to-logical mapping to address timeouts
// 3. implement replay runtime
//
// timeout based on real time is inherently nondeterministic.  three ways
// to solve
//
// - ignore.  drawback: changed semantics.  program may get stuck (e.g.,
//   pbzip2) if they rely on timeout to move forward.
//
// - record timeout, and try to replay (tern approach)
//
// - replace physical time with logical number of sync operations.  can
//   count how many times we get the turn or the turn changed hands, and
//   timeout based on this turn count.  This approach should maintain
//   original semantics if program doesn't rely on physical running time
//   of code.
//
//   optimization, if there's a deadlock (activeq == empty), we just wake
//   up timed waiters in order
//

#include <sys/time.h>
#include <execinfo.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sched.h>
#include "tern/runtime/record-log.h"
#include "tern/runtime/record-runtime.h"
#include "tern/runtime/record-scheduler.h"
#include "signal.h"
#include "helper.h"
#include "tern/space.h"
#include "tern/options.h"
#include "tern/hooks.h"
#include "tern/runtime/rdtsc.h"

#include <fstream>
#include <map>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

// FIXME: these should be in tern/config.h
#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE<199309L)
// Need this for clock_gettime
#  define _POSIX_C_SOURCE (199309L)
#endif

//#define DEBUG_SCHED_WITH_PAXOS
#ifdef DEBUG_SCHED_WITH_PAXOS
#  define debugpaxos(fmt...) do {                   \
     fprintf(stderr, fmt);                       \
     fflush(stderr);                             \
  } while(0)
#else
#  define debugpaxos(fmt...)
#endif

//#define _DEBUG_RECORDER
#ifdef _DEBUG_RECORDER
#  define dprintf(fmt...) do {                   \
     fprintf(stderr, "[%d] ", _S::self());        \
     fprintf(stderr, fmt);                       \
     fflush(stderr);                             \
  } while(0)
#else
#  define dprintf(fmt...)
#endif

//#define __TIME_MEASURE 1

#define ts2ll(x) ((uint64_t) (x).tv_sec * factor + (x).tv_nsec)

using namespace std;

/// Variables for the non-det prfimitive.
pthread_cond_t nonDetCV; /** This cond var does not actually work with other mutexes to do
                                        real cond wait and signal, it just provides a cond var addr for 
                                        _S::wait() and _S::signal(). **/
__thread volatile bool inNonDet = false; /** Per-thread variable to denote whether current thread is within a non_det region. **/
int nNonDetWait = 0; /** This variable is only accessed when a thread gets a turn, so it is safe. **/
tr1::unordered_set<void *> nonDetSyncs; /** Global set to store the sync vars that have ever been accessed within non_det regions of all threads. **/
pthread_spinlock_t nonDetLock; /** a spinlock to protect the acccess to the global set "nonDetSyncs". **/

/** An internal function to record a set of sync vars accessed in non-det regions. **/
void add_non_det_var(void *var) {
  /*pthread_spin_lock(&nonDetLock);
  nonDetSyncs.insert(var);
  pthread_spin_unlock(&nonDetLock);*/
}
 
/** An internal function to check a sync var has not been accessed in non-det regions. **/
bool is_non_det_var(void *var) {
  return false;
  /*pthread_spin_lock(&nonDetLock);
  bool ret = (nonDetSyncs.find(var) != nonDetSyncs.end());
  pthread_spin_unlock(&nonDetLock);
  if (ret)
    fprintf(stderr, "WARN: NON-DET SYNC VAR IS ACCESSED IN DETERMINISTIC REGION.\n");  
  return ret;*/
}

namespace tern {

extern "C" {
  extern int idle_done;
}
static __thread timespec my_time;

/** Pop all send operations with the conn_id. **/
void popSendOps(uint64_t conn_id, unsigned recv_len);

extern "C" void *idle_thread(void*);
extern "C" pthread_t idle_th;
extern "C" pthread_mutex_t idle_mutex;
extern "C" pthread_cond_t idle_cond;

/** This var works with tern_set_base_time(). It is used to record the base 
time for cond_timedwait(), sem_timedwait() and mutex_timedlock() to get
deterministic physical time interval, so that this interval can be 
deterministically converted to logical time interval. **/
static __thread timespec my_base_time = {0, 0};

timespec time_diff(const timespec &start, const timespec &end)
{
  timespec tmp;
  if ((end.tv_nsec-start.tv_nsec) < 0) {
    tmp.tv_sec = end.tv_sec - start.tv_sec - 1;
    tmp.tv_nsec = 1000000000 - start.tv_nsec + end.tv_nsec;
  } else {
    tmp.tv_sec = end.tv_sec - start.tv_sec;
    tmp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return tmp;
}

timespec update_time()
{
  timespec ret;
  timeval tnow;
  gettimeofday(&tnow, NULL);
  ret.tv_sec = tnow.tv_sec;
  ret.tv_nsec = tnow.tv_usec;
  return ret;
/*  timespec start_time;
  if (options::log_sync) {
    clock_gettime(CLOCK_REALTIME , &start_time);
    timespec ret = time_diff(my_time, start_time);
    my_time = start_time; 
    return ret;
  } else
    return start_time;*/
}

void check_options()
{
  if (!options::DMT)
    fprintf(stderr, "WARNING: DMT mode is off. The system won't enter scheduler in "
    "LD_PRELOAD mode!!\n");
  if (!options::RR_ignore_rw_regular_file)
    fprintf(stderr, "WARNING: RR_ignore_rw_regular_file is off, and so we can have "
      "non-determinism on regular file I/O!!\n");
}

void InstallRuntime() {
  check_options();
  Runtime::the = new RecorderRT<RRScheduler>;
}

template <typename _S>
int RecorderRT<_S>::syncWait(void *chan, unsigned timeout) {
  return _S::wait(chan, timeout);
}

template <typename _S>
void RecorderRT<_S>::syncSignal(void *chan, bool all) {
  _S::signal(chan, all);
}

template <typename _S>
int RecorderRT<_S>::absTimeToTurn(const struct timespec *abstime)
{
  // TODO: convert physical time to logical time (number of turns)
  return _S::getTurnCount() + 30; //rand() % 10;
}

int time2turn(uint64_t nsec)
{
  if (!options::launch_idle_thread) {
    fprintf(stderr, "WARN: converting phyiscal time to logical time \
      without launcing idle thread. Please set 'launch_idle_thread' to 1 and then \
      rerun.\n");
    exit(1);
  }

  const uint64_t MAX_REL = (1000000); // maximum number of turns to wait

  uint64_t ret64 = nsec / options::nanosec_per_turn;

  // if result too large, return MAX_REL
  int ret = (ret64 > MAX_REL) ? MAX_REL : (int) ret64;

  return ret;
}

template <typename _S>
int RecorderRT<_S>::relTimeToTurn(const struct timespec *reltime)
{
  if (!reltime) return 0;

  int ret;
  int64_t ns;

  ns = reltime->tv_sec;
  ns = ns * (1000000000) + reltime->tv_nsec;
  ret = time2turn(ns);

  // if result too small or negative, return only (5 * nthread + 1)
  ret = (ret < 5 * _S::nthread + 1) ? (5 * _S::nthread + 1) : ret;
  dprintf("computed turn = %d\n", ret);
  return ret;
}

template <typename _S>
void RecorderRT<_S>::progBegin(void) {
  Logger::progBegin();
}

template <typename _S>
void RecorderRT<_S>::progEnd(void) {
  Logger::progEnd();
}

/*
 *  This is a fake API function that advances clock when all the threads  
 *  are blocked. 
 */
template <typename _S>
void RecorderRT<_S>::idle_sleep(void) {
  _S::getTurn();
  int turn = _S::incTurnCount(__PRETTY_FUNCTION__);
  assert(turn >= 0);
  timespec ts;
  if (options::log_sync)
    Logger::the->logSync(0, syncfunc::tern_idle, turn, ts, ts, ts, true);
  _S::putTurn();
}

template <typename _S>
void RecorderRT<_S>::idle_cond_wait(void) {
  _S::getTurn();
  int turn = _S::incTurnCount(__PRETTY_FUNCTION__);
  assert(turn >= 0);

  /* Currently idle thread must be in runq since it has grabbed the idle_mutex,
    so >=2 means there is at least one real thread in runq as well. */
  if (_S::runq.size() >= 2)
    _S::idleThreadCondWait();
  else
    _S::putTurn();
}

#define PSELF \
  (unsigned)pthread_self()

#define BLOCK_TIMER_START(sync_op, ...) \
  if (options::record_runtime_stat) \
    stat.nInterProcSyncOp++; \
  if (options::enforce_non_det_annotations && inNonDet) { \
    return Runtime::__##sync_op(__VA_ARGS__); \
  } \
  if (_S::interProStart()) { \
    _S::block(__PRETTY_FUNCTION__); \
  }
  //fprintf(stderr, "\n\nBLOCK_TIMER_START ins, pid %d, self %u, tid %d, turnCount %u, function %s\n", getpid(), (unsigned)pthread_self(), _S::self(), _S::turnCount, __FUNCTION__);
// At this moment, since self-thread is ahead of the run queue, so this block() should be very fast.
// TBD: do we need logging here? We can, but not sure whether we need to do this.


#define BLOCK_TIMER_END(syncop, ...) \
  int backup_errno = errno; \
  if (_S::interProEnd()) { \
    _S::wakeup(); \
  } \
  errno = backup_errno;
  //fprintf(stderr, "\n\nBLOCK_TIMER_END pid %d, self %u, tid %d, turnCount %u, function %s\n", getpid(), (unsigned)pthread_self(), _S::self(), _S::turnCount, __FUNCTION__);


#define SCHED_TIMER_START_COMMON \
  unsigned nturn; \
  if (options::enforce_non_det_annotations) \
     assert(!inNonDet); \
  timespec app_time = update_time(); \
  record_rdtsc_op("GET_TURN", "START", 2, NULL); \
  _S::getTurn(); \
  record_rdtsc_op("GET_TURN", "END", 2, NULL); \
  if (options::record_runtime_stat && pthread_self() != idle_th) \
    stat.nDetPthreadSyncOp++;


#define SCHED_TIMER_START \
  SCHED_TIMER_START_COMMON; \
  if (options::sched_with_paxos) \
    schedSocketOp(__FUNCTION__, DMT_REG_SYNC); \
  timespec sched_time = update_time();
  //if (_S::self() != 1)
    //fprintf(stderr, "\n\nSCHED_TIMER_START ins %p, pid %d, self %u, tid %d, turnCount %u, function %s\n", (void *)(long)ins, getpid(), (unsigned)pthread_self(), _S::self(), _S::turnCount, __FUNCTION__);


#define SCHED_TIMER_END_COMMON(syncop, ...) \
  int backup_errno = errno; \
  timespec syscall_time = update_time(); \
  nturn = _S::incTurnCount(__PRETTY_FUNCTION__); \
  if (options::log_sync) \
    Logger::the->logSync(ins, (syncop), nturn = _S::getTurnCount(), app_time, syscall_time, sched_time, true, __VA_ARGS__);
   

#define SCHED_TIMER_END(syncop, ...) \
  SCHED_TIMER_END_COMMON(syncop, __VA_ARGS__); \
  _S::putTurn();\
  errno = backup_errno;
  //if (_S::self() != 1)
    //fprintf(stderr, "\n\nSCHED_TIMER_END ins %p, pid %d, self %u, tid %d, turnCount %u, function %s\n", (void *)(long)ins, getpid(), (unsigned)pthread_self(), _S::self(), _S::turnCount, __FUNCTION__);


// SOCKET_TIMER_START is the same as SCHED_TIMER_START except schedSocketOp(true).
#define SOCKET_TIMER_DECL \
  timespec app_time; \
  timespec sched_time; \
  unsigned nturn;

#define SOCKET_TIMER_START \
  if (options::enforce_non_det_annotations) \
     assert(!inNonDet); \
  app_time = update_time(); \
  record_rdtsc_op("GET_TURN", "START", 2, NULL); \
  _S::getTurn(); \
  record_rdtsc_op("GET_TURN", "END", 2, NULL); \
  if (options::record_runtime_stat && pthread_self() != idle_th) \
    stat.nDetPthreadSyncOp++; \
  sched_time = update_time();
  //if (_S::self() != 1) \
    //fprintf(stderr, "\n\nSOCKET_TIMER_START time <%ld.%ld> ins %p, pid %d, self %u, tid %d, turnCount %u, function %s\n", \
      //sched_time.tv_sec, sched_time.tv_nsec, (void *)(long)ins, getpid(), (unsigned)pthread_self(), _S::self(), _S::turnCount, __FUNCTION__);


// SOCKET_TIMER_END is the same as SCHED_TIMER_END.
#define SOCKET_TIMER_END(syncop, ...) \
  SCHED_TIMER_END_COMMON(syncop, __VA_ARGS__); \
  _S::putTurn();\
  errno = backup_errno;
  //if (_S::self() != 1) \
    //fprintf(stderr, "\n\nSOCKET_TIMER_END time <%ld.%ld> ins %p, pid %d, self %u, tid %d, turnCount %u, function %s\n", \
      //syscall_time.tv_sec, syscall_time.tv_nsec, (void *)(long)ins, getpid(), (unsigned)pthread_self(), _S::self(), _S::turnCount, __FUNCTION__);


#define SCHED_TIMER_THREAD_END(syncop, ...) \
  SCHED_TIMER_END_COMMON(syncop, __VA_ARGS__); \
  _S::putTurn(true);\
  errno = backup_errno; 
  

#define SCHED_TIMER_FAKE_END(syncop, ...) \
  nturn = _S::incTurnCount(__PRETTY_FUNCTION__); \
  timespec fake_time = update_time(); \
  if (options::log_sync) \
    Logger::the->logSync(ins, syncop, nturn, app_time, fake_time, sched_time, /* before */ false, __VA_ARGS__); 


template <typename _S>
void RecorderRT<_S>::printStat(){
  // We must get turn, and print, and then put turn. This is a solid way of 
  // getting deterministic runtime stat.
  _S::getTurn();
  if (options::record_runtime_stat)
    stat.print();
  _S::incTurnCount(__PRETTY_FUNCTION__);
  _S::putTurn();
}
  
template <typename _S>
void RecorderRT<_S>::threadBegin(void) {
  pthread_t th = pthread_self();
  unsigned ins = INVALID_INSID;

  if(_S::self() != _S::MainThreadTid) {
    sem_wait(&thread_begin_sem);
    _S::self(th);
    sem_post(&thread_begin_done_sem);
  }
  assert(_S::self() != _S::InvalidTid);

  SCHED_TIMER_START;
  
  app_time.tv_sec = app_time.tv_nsec = 0;
  Logger::threadBegin(_S::self());

  SCHED_TIMER_END(syncfunc::tern_thread_begin, (uint64_t)th);
}

template <typename _S>
void RecorderRT<_S>::threadEnd(unsigned ins) {
  SCHED_TIMER_START;
  pthread_t th = pthread_self();

  SCHED_TIMER_THREAD_END(syncfunc::tern_thread_end, (uint64_t)th);
  
  Logger::threadEnd();
}

/// The pthread_create wrapper solves three problems.
///
/// Problem 1.  We must assign a logical tern tid to the new thread while
/// holding turn, or multiple newly created thread could get their logical
/// tids nondeterministically.  To do so, we assign a logical tid to a new
/// thread in the thread that creates the new thread.
///
/// If we were to assign this logical id in the new thread itself, we may
/// run into nondeterministic runs, as illustrated by the following
/// example
///
///       t0        t1           t2            t3
///    getTurn();
///    create t2
///    putTurn();
///               getTurn();
///               create t3
///               putTurn();
///                                         getTurn();
///                                         get turn tid 2
///                                         putTurn();
///                            getTurn();
///                            get turn tid 3
///                            putTurn();
///
/// in a different run, t1 may run first and get turn tid 2.
///
/// Problem 2.  When a child thread is created, the child thread may run
/// into a getTurn() before the parent thread has assigned a logical tid
/// to the child thread.  This causes getTurn to refer to self_tid, which
/// is undefined.  To solve this problem, we use @thread_begin_sem to
/// create a thread in suspended mode, until the parent thread has
/// assigned a logical tid for the child thread.
///
/// Problem 3.  We can use one semaphore to create a child thread
/// suspended.  However, when there are two pthread_create() calls, we may
/// still run into cases where a child thread tries to get its
/// thread-local tid but gets -1.  consider
///
///       t0        t1           t2            t3
///    getTurn();
///    create t2
///    sem_post(&thread_begin_sem)
///    putTurn();
///               getTurn();
///               create t3
///                                         sem_wait(&thread_begin_sem);
///                                         self_tid = TidMap[pthread_self()]
///               sem_post(&thread_begin_sem)
///               putTurn();
///                                         getTurn();
///                                         get turn tid 2
///                                         putTurn();
///                            sem_wait(&thread_begin_sem);
///                            self_tid = TidMap[pthread_self()]
///                            getTurn();
///                            get turn tid 3
///                            putTurn();
///
/// The crux of the problem is that multiple sem_post can pair up with
/// multiple sem_down in different ways.  We solve this problem using
/// another semaphore, thread_begin_done_sem.
///
template <typename _S>
int RecorderRT<_S>::pthreadCreate(unsigned ins, int &error, pthread_t *thread,
         pthread_attr_t *attr, void *(*thread_func)(void*), void *arg) {
  int ret;
  SCHED_TIMER_START;

  ret = __tern_pthread_create(thread, attr, thread_func, arg);
  assert(!ret && "failed sync calls are not yet supported!");
  _S::create(*thread);

  SCHED_TIMER_END(syncfunc::pthread_create, (uint64_t)*thread, (uint64_t) ret);
 
  sem_post(&thread_begin_sem);
  sem_wait(&thread_begin_done_sem);
  
  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadJoin(unsigned ins, int &error, pthread_t th, void **rv) {
  int ret;
  SCHED_TIMER_START;
  // NOTE: can actually use if(!_S::zombie(th)) for DMT schedulers because
  // their wait() won't return until some thread signal().
  while(!_S::zombie(th)) {
    /* Don't call syncWait here, but this scheduler wait, because there is a pairwise 
      signal() in putTurn with thread_end. */
    _S::wait((void*)th);
  }
  errno = error;

  ret = pthread_join(th, rv);
  /*if(options::pthread_tryjoin) {
    // FIXME: sometimes a child process gets stuck in
    // pthread_join(idle_th, NULL) in __tern_prog_end(), perhaps because
    // idle_th has become zombie and since the program is exiting, the
    // zombie thread is reaped.
    for(int i=0; i<10; ++i) {
      ret = pthread_tryjoin_np(th, rv);
      if(ret != EBUSY)
        break;
      ::usleep(10);
    }
    if(ret == EBUSY) {
      dprintf("can't join thread; try canceling it instead");
      pthread_cancel(th);
      ret = 0;
    }
  } else {
    ret = pthread_join(th, rv);
  }*/

  error = errno;
  assert(!ret && "failed sync calls are not yet supported!");
  _S::join(th);

  SCHED_TIMER_END(syncfunc::pthread_join, (uint64_t)th);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__pthread_detach(unsigned insid, int &error, pthread_t th) {
  BLOCK_TIMER_START(pthread_detach, insid, error, th);
  int ret = Runtime::__pthread_detach(insid, error, th);
  fprintf(stderr, "CRANE: executes pthread_detach.\n");
  BLOCK_TIMER_END(syncfunc::pthread_detach, (uint64_t)ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadMutexInit(unsigned ins, int &error, pthread_mutex_t *mutex, const  pthread_mutexattr_t *mutexattr)
{
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)mutex);
    dprintf("Thread tid %d, self %u is calling non-det pthread_mutex_init.\n", _S::self(), (unsigned)pthread_self());
    return Runtime::__pthread_mutex_init(ins, error, mutex, mutexattr);
  }
  SCHED_TIMER_START;
  errno = error;
  ret = pthread_mutex_init(mutex, mutexattr);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_mutex_init, (uint64_t)ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadMutexDestroy(unsigned ins, int &error, pthread_mutex_t *mutex)
{
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)mutex);
    return Runtime::__pthread_mutex_destroy(ins, error, mutex);
  }
  SCHED_TIMER_START;
  errno = error;
  ret = pthread_mutex_destroy(mutex);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_mutex_destroy, (uint64_t)ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadMutexLockHelper(pthread_mutex_t *mu, unsigned timeout) {
  int ret;
  while((ret=pthread_mutex_trylock(mu))) {
    assert(ret==EBUSY && "failed sync calls are not yet supported!");
    ret = syncWait(mu, timeout);
    if(ret == ETIMEDOUT)
      return ETIMEDOUT;
  }
  return 0;
}

template <typename _S>
int RecorderRT<_S>::pthreadRWLockWrLockHelper(pthread_rwlock_t *rwlock, unsigned timeout) {
  int ret;
  while((ret=pthread_rwlock_trywrlock(rwlock))) {
    assert(ret==EBUSY && "failed sync calls are not yet supported!");
    ret = syncWait(rwlock, timeout);
    if(ret == ETIMEDOUT)
      return ETIMEDOUT;
  }
  return 0;
}

template <typename _S>
int RecorderRT<_S>::pthreadRWLockRdLockHelper(pthread_rwlock_t *rwlock, unsigned timeout) {
  int ret;
  while((ret=pthread_rwlock_tryrdlock(rwlock))) {
    assert(ret==EBUSY && "failed sync calls are not yet supported!");
    ret = syncWait(rwlock, timeout);
    if(ret == ETIMEDOUT)
      return ETIMEDOUT;
  }
  return 0;
}

template <typename _S>
int RecorderRT<_S>::pthreadMutexLock(unsigned ins, int &error, pthread_mutex_t *mu) {
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)mu);
    dprintf("Ins %p :   Thread tid %d, self %u is calling non-det pthread_mutex_lock.\n", (void *)ins, _S::self(), (unsigned)pthread_self());
    return Runtime::__pthread_mutex_lock(ins, error, mu);
  }
  SCHED_TIMER_START;
  errno = error;
  pthreadMutexLockHelper(mu);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_mutex_lock, (uint64_t)mu);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_rdlock(unsigned ins, int &error, pthread_rwlock_t *rwlock)
{
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_rdlock(rwlock);
  }
  SCHED_TIMER_START;
  errno = error;
  pthreadRWLockRdLockHelper(rwlock);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_rwlock_rdlock, (uint64_t)rwlock);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_wrlock(unsigned ins, int &error, pthread_rwlock_t *rwlock)
{
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_wrlock(rwlock);
  }
  SCHED_TIMER_START;
  errno = error;
  pthreadRWLockWrLockHelper(rwlock);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_rwlock_wrlock, (uint64_t)rwlock);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_tryrdlock(unsigned ins, int &error, pthread_rwlock_t *rwlock)
{
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_tryrdlock(rwlock);
  }
  SCHED_TIMER_START;
  errno = error;
  int ret = pthread_rwlock_trywrlock(rwlock); //  FIXME now using wrlock for all rdlock
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_rwlock_tryrdlock, (uint64_t)rwlock, (uint64_t) ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_trywrlock(unsigned ins, int &error, pthread_rwlock_t *rwlock)
{
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_trywrlock(rwlock);
  }
  SCHED_TIMER_START;
  errno = error;
  int ret = pthread_rwlock_trywrlock(rwlock); 
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_rwlock_trywrlock, (uint64_t)rwlock, (uint64_t) ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_unlock(unsigned ins, int &error, pthread_rwlock_t *rwlock)
{
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_unlock(rwlock);
  }
  SCHED_TIMER_START;

  errno = error;
  ret = pthread_rwlock_unlock(rwlock);
  error = errno;
  syncSignal(rwlock);
 
  SCHED_TIMER_END(syncfunc::pthread_rwlock_unlock, (uint64_t)rwlock, (uint64_t) ret);

  return ret;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_destroy(unsigned ins, int &error, pthread_rwlock_t *rwlock) {
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_destroy(rwlock);
  }
  SCHED_TIMER_START;
  errno = error;
  int ret = pthread_rwlock_destroy(rwlock); 
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_rwlock_destroy, (uint64_t)rwlock, (uint64_t) ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__pthread_rwlock_init(unsigned ins, int &error, pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) {
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)rwlock);
    return pthread_rwlock_init(rwlock, attr);
  }
  SCHED_TIMER_START;
  errno = error;
  int ret = pthread_rwlock_init(rwlock, attr); 
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_rwlock_init, (uint64_t)rwlock, attr, (uint64_t) ret);
  return ret;
}

/// instead of looping to get lock as how we implement the regular lock(),
/// here just trylock once and return.  this preserves the semantics of
/// trylock().
template <typename _S>
int RecorderRT<_S>::pthreadMutexTryLock(unsigned ins, int &error, pthread_mutex_t *mu) {
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)mu);
    return pthread_mutex_trylock(mu);
  }
  SCHED_TIMER_START;
  errno = error;
  ret = pthread_mutex_trylock(mu);
  error = errno;
  assert((!ret || ret==EBUSY)
         && "failed sync calls are not yet supported!");
  SCHED_TIMER_END(syncfunc::pthread_mutex_trylock, (uint64_t)mu, (uint64_t) ret);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadMutexTimedLock(unsigned ins, int &error, pthread_mutex_t *mu,
                                                const struct timespec *abstime) {
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)mu);
    return pthread_mutex_timedlock(mu, abstime);
  }
  if(abstime == NULL)
    return pthreadMutexLock(ins, error, mu);

  timespec cur_time, rel_time;
  if (my_base_time.tv_sec == 0) {
    fprintf(stderr, "WARN: pthread_mutex_timedlock has a non-det timeout. \
    Please use it with tern_set_base_timespec().\n");
    clock_gettime(CLOCK_REALTIME, &cur_time);
  } else {
    cur_time.tv_sec = my_base_time.tv_sec;
    cur_time.tv_nsec = my_base_time.tv_nsec;
  }
  rel_time = time_diff(cur_time, *abstime);

  SCHED_TIMER_START;
  unsigned timeout = _S::getTurnCount() + relTimeToTurn(&rel_time);
  errno = error;
  int ret = pthreadMutexLockHelper(mu, timeout);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_mutex_timedlock, (uint64_t)mu, (uint64_t) ret);
 
  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadMutexUnlock(unsigned ins, int &error, pthread_mutex_t *mu){
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)mu);
    dprintf("Thread tid %d, self %u is calling non-det pthread_mutex_unlock.\n", _S::self(), (unsigned)pthread_self());
    return Runtime::__pthread_mutex_unlock(ins, error, mu);
  }
  //fprintf(stderr, "pthreadMutexUnlock1\n");
  SCHED_TIMER_START;
  //fprintf(stderr, "pthreadMutexUnlock2\n");
  errno = error;
  ret = pthread_mutex_unlock(mu);
  error = errno;
  //fprintf(stderr, "pthreadMutexUnlock3\n");
  assert(!ret && "failed sync calls are not yet supported!");
  syncSignal(mu);
  //fprintf(stderr, "pthreadMutexUnlock4\n");
  SCHED_TIMER_END(syncfunc::pthread_mutex_unlock, (uint64_t)mu, (uint64_t) ret);

  return ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadBarrierInit(unsigned ins, int &error, pthread_barrier_t *barrier,
                                       unsigned count) {
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)barrier);
    return pthread_barrier_init(barrier, NULL, count);
  }
  SCHED_TIMER_START;
  errno = error;
  ret = pthread_barrier_init(barrier, NULL, count);
  error = errno;
  assert(!ret && "failed sync calls are not yet supported!");
  assert(barriers.find(barrier) == barriers.end()
         && "barrier already initialized!");
  barriers[barrier].count = count;
  barriers[barrier].narrived = 0;

  SCHED_TIMER_END(syncfunc::pthread_barrier_init, (uint64_t)barrier, (uint64_t) count);
 
  return ret;
}

/// barrier_wait has a similar problem to pthread_cond_wait (see below).
/// we want to avoid the head of queue block, so must call syncWait(ba) and
/// give up turn before calling pthread_barrier_wait.  However, when the
/// last thread arrives, we must wake up the waiting threads.
///
/// solution: we keep track of the barrier count by ourselves, so that the
/// last thread arriving at the barrier can figure out that it is the last
/// thread, and wakes up all other threads.
///
template <typename _S>
int RecorderRT<_S>::pthreadBarrierWait(unsigned ins, int &error, 
                                       pthread_barrier_t *barrier) {
  /// Note: the syncSignal() operation has to be done while the thread has the
  /// turn; otherwise two independent syncSignal() operations on two
  /// independent barriers can be nondeterministic.  e.g., suppose two
  /// barriers ba1 and ba2 each has count 1.
  ///
  ///       t0                        t1
  ///
  /// getTurn()
  /// syncWait(ba1);
  ///                         getTurn()
  ///                         syncWait(ba1);
  /// barrier_wait(ba1)
  ///                         barrier_wait(ba2)
  /// syncSignal(ba1)             syncSignal(ba2)
  ///
  /// these two syncSignal() ops can be nondeterministic, causing threads to be
  /// added to the activeq in different orders
  ///
  
    
  
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)barrier);
    return pthread_barrier_wait(barrier);
  }
  SCHED_TIMER_START;
  SCHED_TIMER_FAKE_END(syncfunc::pthread_barrier_wait, (uint64_t)barrier);
  
  barrier_map::iterator bi = barriers.find(barrier);
  assert(bi!=barriers.end() && "barrier is not initialized!");
  barrier_t &b = bi->second;

  ++ b.narrived;

//#define xxx
#ifdef xxx
  fprintf(stderr, "thread %d arrives at barrier, b.count = %d, b.arrvied = %d\n", 
    _S::self(), (int) b.count, (int) b.narrived);
  fflush(stderr);
#endif

  assert(b.narrived <= b.count && "barrier overflow!");
  if(b.count == b.narrived) {
    b.narrived = 0; // barrier may be reused
    syncSignal(barrier, /*all=*/true);
    // according to the man page of pthread_barrier_wait, one of the
    // waiters should return PTHREAD_BARRIER_SERIAL_THREAD, instead of 0
    ret = PTHREAD_BARRIER_SERIAL_THREAD;
#ifdef xxx
    fprintf(stderr, "thread %d claims itself as the last one, b.count = %d, b.arrvied = %d\n", 
      _S::self(), (int) b.count, (int) b.narrived);
    fflush(stderr);
#endif

    _S::putTurn();  // this gives _first and _second different turn numbers.
    _S::getTurn();
  } else {
    ret = 0;
    syncWait(barrier);
  }
  sched_time = update_time();
#ifdef xxx
  fprintf(stderr, "thread %d leaves barrier\n", _S::self());
  fflush(stderr);
#endif

  SCHED_TIMER_END(syncfunc::pthread_barrier_wait, (uint64_t)barrier);

  return ret;
}

// FIXME: the handling of the EBUSY case seems gross
template <typename _S>
int RecorderRT<_S>::pthreadBarrierDestroy(unsigned ins, int &error, 
                                          pthread_barrier_t *barrier) {
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)barrier);
    return pthread_barrier_destroy(barrier);
  }
  SCHED_TIMER_START;
  errno = error;
  ret = pthread_barrier_destroy(barrier);
  error = errno;

  // pthread_barrier_destroy returns EBUSY if the barrier is still in use
  assert((!ret || ret==EBUSY) && "failed sync calls are not yet supported!");
  if(!ret) {
    barrier_map::iterator bi = barriers.find(barrier);
    assert(bi != barriers.end() && "barrier not initialized!");
    barriers.erase(bi);
  }
  
  SCHED_TIMER_END(syncfunc::pthread_barrier_destroy, (uint64_t)barrier, (uint64_t) ret);
   
  return ret;
}

/// The issues with pthread_cond_wait()
///
/// ------ First issue: deadlock. Normally, we'd want to do
///
///   getTurn();
///   pthread_cond_wait(&cv, &mu);
///   putTurn();
///
/// However, pthread_cond_wait blocks, so we won't call putTurn(), thus
/// deadlocking the entire system.  And there is (should be) no
/// cond_trywait, unlike in the case of pthread_mutex_t.
///
///
/// ------ A naive solution is to putTurn before calling pthread_cond_wait
///
///   getTurn();
///   putTurn();
///   pthread_cond_wait(&cv, &mu);
///
/// However, this is nondeterministic.  Two nondeterminism scenarios:
///
/// 1: race with a pthread_mutex_lock() (or trylock()).  Depending on the
/// timing, the lock() may or may not succeed.
///
///   getTurn();
///   putTurn();
///                                     getTurn();
///   pthread_cond_wait(&cv, &mu);      pthread_mutex_lock(&mu) (or trylock())
///                                     putTurn();
///
/// 2: race with pthread_cond_signal() (or broadcast()).  Depending on the
/// timing, the signal may or may not be received.  Note the code below uses
/// "naked signal" without holding the lock, which is wrong, but despite so,
/// we still have to be deterministic.
///
///   getTurn();
///   putTurn();
///                                      getTurn();
///   pthread_cond_wait(&cv, &mu);       pthread_cond_signal(&cv) (or broadcast())
///                                      putTurn();
///
///
/// ------ First solution: replace mu with the scheduler mutex
///
///   getTurn();
///   pthread_mutex_unlock(&mu);
///   syncSignal(&mu);
///   waitFirstHalf(&cv); // put self() to waitq for &cv, but don't wait on
///                       // any internal sync obj or release scheduler lock
///                       // because we'll wait on the cond var in user
///                       // program
///   pthread_cond_wait(&cv, &schedulerLock);
///   getTurnNU(); // get turn without lock(&schedulerLock), but
///                // unlock(&schedulerLock) when returns
///
///   pthreadMutexLockHelper(mu); // call lock wrapper
///   putTurn();
///
/// This solution solves race 1 & 2 at the same time because getTurn has to
/// wait until schedulerLock is released.
///
///
/// ------ Issue with the first solution: deadlock.
///
/// The first solution may still deadlock because the thread we
/// deterministically choose to wake up may be different from the one
/// chosen nondeterministically by pthread.
///
/// consider:
///
///   pthread_cond_wait(&cv, &schedulerLock);      pthread_cond_wait(&cv, &schedulerLock);
///   getTurnWithoutLock();                        getTurnWithoutLock()
///   ret = pthread_mutex_trylock(&mu);            ret = pthread_mutex_trylock(&mu);
///   ...                                          ...
///   putTurn();                                   putTurn();
///
/// When a thread calls pthread_cond_signal, suppose pthread chooses to
/// wake up the second thread.  However, the first thread may be the one
/// that gets the turn first.  Therefore the system would deadlock.
///
///
/// ------- Second solution: replace pthread_cond_signal with
/// pthread_cond_broadcast, which wakes up all threads, and then the
/// thread that gets the turn first would proceed first to get mu.
///
/// pthread_cond_signal(cv):
///
///   getTurnLN();  // lock(&schedulerLock) but does not unlock(&schedulerLock)
///   pthread_cond_broadcast(cv);
///   signalNN(cv); // wake up the first thread waiting on cv; need to hold
///                 // schedulerLock because signalNN touches wait queue
///   putTurnNU();  // no lock() but unlock(&schedulerLock)
///
/// This is the solution currently implement.
///
///
/// ------- Issues with the second solution: changed pthread_cond_signal semantics
///
/// Once we change cond_signal to cond_broadcast, all woken up threads can
/// proceed after the first thread releases mu.  This differs from the
/// cond var semantics where one signal should only wake up one thread, as
/// seen in the Linux man page:
///
///   !pthread_cond_signal! restarts one of the threads that are waiting
///   on the condition variable |cond|. If no threads are waiting on
///   |cond|, nothing happens. If several threads are waiting on |cond|,
///   exactly one is restarted, but it is not specified which.
///
/// If the application program correctly uses mesa-style cond var, i.e.,
/// the woken up thread re-checks the condition using while, waking up
/// more than one thread would not be an issue.  In addition, according
/// to the IEEE/The Open Group, 2003, PTHREAD_COND_BROADCAST(P):
///
///   In a multi-processor, it may be impossible for an implementation of
///   pthread_cond_signal() to avoid the unblocking of more than one
///   thread blocked on a condition variable."
///
/// However, we have to be deterministic despite program errors (again).
///
///
/// ------- Third (proposed, not yet implemented) solution: replace cv
///         with our own cv in the actual pthread_cond_wait
///
/// pthread_cond_wait(cv, mu):
///
///   getTurn();
///   pthread_mutex_unlock(&mu);
///   syncSignal(&mu);
///   waitFirstHalf(&cv);
///   putTurnLN();
///   pthread_cond_wait(&waitcv[self()], &schedulerLock);
///   getTurnNU();
///   pthreadMutexLockHelper(mu);
///   putTurn();
///
/// pthread_cond_signal(&cv):
///
///   getTurnLN();
///   signalNN(&cv) // wake up precisely the first thread with waitVar == chan
///   pthread_cond_signal(&cv); // no op
///   putTurnNU();
///
/// ----- Fourth (proposed, not yet implemented) solution: re-implement
///       pthread cv all together
///
/// A closer look at the code shows that we're not really using the
/// original conditional variable at all.  That is, no thread ever waits
/// on the original cond_var (ever calls cond_wait() on the cond var).  We
/// may as well skip cond_signal.  That is, we're reimplementing cond vars
/// with our own queues, lock and cond vars.
///
/// This observation motives our next design: re-implementing cond var on
/// top of semaphore, so that we can get rid of the schedulerLock.  We can
/// implement the scheduler functions as follows:
///
/// getTurn():
///   sem_wait(&semOfThread);
///
/// putTurn():
///   move self to end of active q
///   find sem of head of q
///   sem_post(&semOfHead);
///
/// wait():
///   move self to end of wait q
///   find sem of head of q
///   sem_post(&sem_of_head);
///
/// We can then implement pthread_cond_wait and pthread_cond_signal as follows:
///
/// pthread_cond_wait(&cv, &mu):
///   getTurn();
///   pthread_mutex_unlock(&mu);
///   syncSignal(&mu);
///   wait(&cv);
///   putTurn();
///
///   getTurn();
///   pthreadMutexLockHelper(&mu);
///   putTurn();
///
/// pthread_cond_signal(&cv):
///   getTurn();
///   syncSignal(&cv);
///   putTurn();
///
/// One additional optimization we can do for programs that do sync
/// frequently (1000 ops > 1 second?): replace the semaphores with
/// integer flags.
///
///   sem_wait(semOfThread) ==>  while(flagOfThread != 1);
///   sem_post(semOfHead)   ==>  flagOfHead = 1;
///
///
///  ----- Fifth (proposed, probably not worth implementing) solution: can
///        be more aggressive and implement more stuff on our own (mutex,
///        barrier, etc), but probably unnecessary.  skip sleep and
///        barrier_wait help the most
///
///
///  ---- Summary
///
///  solution 2: lock + cv + cond_broadcast.  deterministic if application
///  code use while() with cond_wait, same sync var state except short
///  periods of time within cond_wait.  the advantage is that it ensures
///  that all sync vars have the same internal states, in case the
///  application code breaks abstraction boundaries and reads the internal
///  states of the sync vars.
///
///  solution 3: lock + cv + replace cv.  deterministic, but probably not
///  as good as semaphore since the schedulerLock seems unnecessary.  the
///  advantage is that it ensures that all sync vars except original cond
///  vars have the same internal states
///
///  solution 4: semaphore.  deterministic, good if sync frequency
///  low.
///
///  solution 4 optimization: flag.  deterministic, good if sync
///  frequency high
///
///  solution 5: probably not worth it
///
template <typename _S>
int RecorderRT<_S>::pthreadCondWait(unsigned ins, int &error, 
                                    pthread_cond_t *cv, pthread_mutex_t *mu){
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)cv);
    add_non_det_var((void *)mu);
    return pthread_cond_wait(cv, mu);
  }
  SCHED_TIMER_START;
  pthread_mutex_unlock(mu);
  syncSignal(mu);

  SCHED_TIMER_FAKE_END(syncfunc::pthread_cond_wait, (uint64_t)cv, (uint64_t)mu);
  syncWait(cv);
  sched_time = update_time();
  errno = error;
  pthreadMutexLockHelper(mu);
  error = errno;
  
  SCHED_TIMER_END(syncfunc::pthread_cond_wait, (uint64_t)cv, (uint64_t)mu);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::pthreadCondTimedWait(unsigned ins, int &error, 
    pthread_cond_t *cv, pthread_mutex_t *mu, const struct timespec *abstime){

  int saved_ret = 0;

  if(abstime == NULL)
    return pthreadCondWait(ins, error, cv, mu);

  timespec cur_time, rel_time;
  if (my_base_time.tv_sec == 0) {
    fprintf(stderr, "pthread_cond_timedwait is called. Please add tern_set_base_timespec() if possible.\n");
    clock_gettime(CLOCK_REALTIME, &cur_time);
  } else {
    cur_time.tv_sec = my_base_time.tv_sec;
    cur_time.tv_nsec = my_base_time.tv_nsec;
  }
  rel_time = time_diff(cur_time, *abstime);

  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)cv);
    add_non_det_var((void *)mu);
    return pthread_cond_timedwait(cv, mu, abstime);
  }
  SCHED_TIMER_START;
  pthread_mutex_unlock(mu);

  SCHED_TIMER_FAKE_END(syncfunc::pthread_cond_timedwait, (uint64_t)cv, (uint64_t)mu, (uint64_t) 0);

  syncSignal(mu);
  unsigned nTurns = relTimeToTurn(&rel_time);
  dprintf("Tid %d pthreadCondTimedWait physical time interval %ld.%ld, logical turns %u\n",
    _S::self(), (long)rel_time.tv_sec, (long)rel_time.tv_nsec, nTurns);
  unsigned timeout = _S::getTurnCount() + nTurns;
  saved_ret = ret = syncWait(cv, timeout);
  dprintf("timedwait return = %d, after %d turns\n", ret, _S::getTurnCount() - nturn);

  sched_time = update_time();
  errno = error;
  pthreadMutexLockHelper(mu);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_cond_timedwait, (uint64_t)cv, (uint64_t)mu, (uint64_t) saved_ret);

  return saved_ret;
}

template <typename _S>
int RecorderRT<_S>::pthreadCondSignal(unsigned ins, int &error, pthread_cond_t *cv){
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)cv);
    return pthread_cond_signal(cv);
  }
  //fprintf(stderr, "pthreadCondSignal start...\n");
  SCHED_TIMER_START;
  //fprintf(stderr, "pthreadCondSignal start got turn...\n");
  syncSignal(cv);
  //fprintf(stderr, "pthreadCondSignal start got turn2...\n");
  SCHED_TIMER_END(syncfunc::pthread_cond_signal, (uint64_t)cv);
  //fprintf(stderr, "pthreadCondSignal start put turn...\n");
  return 0;
}

template <typename _S>
int RecorderRT<_S>::pthreadCondBroadcast(unsigned ins, int &error, pthread_cond_t*cv){
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)cv);
    return pthread_cond_broadcast(cv);
  }
  SCHED_TIMER_START;
  syncSignal(cv, /*all=*/true);
  SCHED_TIMER_END(syncfunc::pthread_cond_broadcast, (uint64_t)cv);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::semWait(unsigned ins, int &error, sem_t *sem) {
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)sem);
    //fprintf(stderr, "non det sem wait...\n");
    return Runtime::__sem_wait(ins, error, sem);
  }
  SCHED_TIMER_START;
  while((ret=sem_trywait(sem)) != 0) {
    // WTH? pthread_mutex_trylock returns EBUSY if lock is held, yet
    // sem_trywait returns -1 and sets errno to EAGAIN if semaphore is not
    // available
    assert(errno==EAGAIN && "failed sync calls are not yet supported!");
    syncWait(sem);
  }
  SCHED_TIMER_END(syncfunc::sem_wait, (uint64_t)sem);

  return 0;
}

template <typename _S>
int RecorderRT<_S>::semTryWait(unsigned ins, int &error, sem_t *sem) {
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)sem);
    return sem_trywait(sem);
  }
  SCHED_TIMER_START;
  errno = error;
  ret = sem_trywait(sem);
  error = errno;
  if(ret != 0)
    assert(errno==EAGAIN && "failed sync calls are not yet supported!");
  SCHED_TIMER_END(syncfunc::sem_trywait, (uint64_t)sem, (uint64_t)ret);
 
  return ret;
}

template <typename _S>
int RecorderRT<_S>::semTimedWait(unsigned ins, int &error, sem_t *sem,
                                     const struct timespec *abstime) {
  int saved_err = 0;
  if(abstime == NULL)
    return semWait(ins, error, sem);

  timespec cur_time, rel_time;
  if (my_base_time.tv_sec == 0) {
    fprintf(stderr, "WARN: sem_timedwait has a non-det timeout. \
    Please add tern_set_base_timespec().\n");
    clock_gettime(CLOCK_REALTIME, &cur_time);
  } else {
    cur_time.tv_sec = my_base_time.tv_sec;
    cur_time.tv_nsec = my_base_time.tv_nsec;
  }
  rel_time = time_diff(cur_time, *abstime);
  
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)sem);
    return sem_timedwait(sem, abstime);
  }
  SCHED_TIMER_START;
  
  unsigned timeout = _S::getTurnCount() + relTimeToTurn(&rel_time);
  while((ret=sem_trywait(sem))) {
    assert(errno==EAGAIN && "failed sync calls are not yet supported!");
    ret = syncWait(sem, timeout);
    if(ret == ETIMEDOUT) {
      ret = -1;
      saved_err = ETIMEDOUT;
      error = ETIMEDOUT;
      break;
    }
  }
  SCHED_TIMER_END(syncfunc::sem_timedwait, (uint64_t)sem, (uint64_t)ret);

  errno = saved_err;
  return ret;
}

template <typename _S>
int RecorderRT<_S>::semPost(unsigned ins, int &error, sem_t *sem){
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)sem);
    return Runtime::__sem_post(ins, error, sem);
  }
  SCHED_TIMER_START;
  ret = sem_post(sem);
  assert(!ret && "failed sync calls are not yet supported!");
  syncSignal(sem);
  SCHED_TIMER_END(syncfunc::sem_post, (uint64_t)sem, (uint64_t)ret);
 
  return 0;
}

template <typename _S>
int RecorderRT<_S>::semInit(unsigned ins, int &error, sem_t *sem, int pshared, unsigned int value){
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)sem);
    return Runtime::__sem_init(ins, error, sem, pshared, value);
  }
  SCHED_TIMER_START;
  ret = sem_init(sem, pshared, value);
  assert(!ret && "failed sync calls are not yet supported!");
  SCHED_TIMER_END(syncfunc::sem_init, (uint64_t)sem, (uint64_t)ret);

  return 0;
}

template <typename _S>
void RecorderRT<_S>::lineupInit(long opaque_type, unsigned count, unsigned timeout_turns) {
  unsigned ins = opaque_type;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)opaque_type);
    return;
  }
  SCHED_TIMER_START;
  //fprintf(stderr, "lineupInit opaque_type %p, count %u, timeout %u\n", (void *)opaque_type, count, timeout_turns);
  if (refcnt_bars.find(opaque_type) != refcnt_bars.end()) {
    fprintf(stderr, "refcnt barrier %p already initialized!\n", (void *)opaque_type);
    assert(false);
  }
  refcnt_bars[opaque_type].count = count;
  refcnt_bars[opaque_type].nactive = 0;
  refcnt_bars[opaque_type].timeout = timeout_turns;
  refcnt_bars[opaque_type].setArriving();
  SCHED_TIMER_END(syncfunc::tern_lineup_init, (uint64_t)opaque_type, (uint64_t) count, (uint64_t) timeout_turns);
}

template <typename _S>
void RecorderRT<_S>::lineupDestroy(long opaque_type) {
  unsigned ins = opaque_type;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)opaque_type);
    return;
  }
  SCHED_TIMER_START;
  //fprintf(stderr, "lineupDestroy opaque_type %p\n", (void *)opaque_type);
  assert(refcnt_bars.find(opaque_type) != refcnt_bars.end() && "refcnt barrier is not initialized!");
  refcnt_bars[opaque_type].count = 0;
  refcnt_bars[opaque_type].nactive = 0;
  refcnt_bars[opaque_type].timeout = 0;
  refcnt_bars[opaque_type].nSuccess = 0;
  refcnt_bars[opaque_type].nTimeout = 0;
  refcnt_bars[opaque_type].setArriving();  
  refcnt_bars.erase(opaque_type);
  SCHED_TIMER_END(syncfunc::tern_lineup_destroy, (uint64_t)opaque_type);
}

template <typename _S>
void RecorderRT<_S>::lineupStart(long opaque_type) {
  unsigned ins = opaque_type;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)opaque_type);
    return;
  }
  //fprintf(stderr, "lineupStart opaque_type %p, tid %d, waiting for turn...\n", (void *)opaque_type, _S::self());

  record_rdtsc_op(__FUNCTION__, "START", 1, NULL); // Record rdtsc start, disabled by default.

  SCHED_TIMER_START;
  refcnt_bar_map::iterator bi = refcnt_bars.find(opaque_type);
  assert(bi != refcnt_bars.end() && "refcnt barrier is not initialized!");
  ref_cnt_barrier_t &b = bi->second;
  b.nactive++;  
  //fprintf(stderr, "lineupStart opaque_type %p, tid %d, count %d, nactive %u\n", (void *)opaque_type, _S::self(), b.count, b.nactive);

  if (b.nactive == b.count) {
    if (b.isArriving()) {
      // full, do not reset "nactive", since we are ref-counting barrier..
      /*b.nSuccess++;
      if (b.nSuccess%1000 == 0)
        fprintf(stderr, "lineupStart opaque_type %p, tid %d, full and success (%ld:%ld)!\n",
          (void *)opaque_type, _S::self(), b.nSuccess, b.nTimeout);*/
      if (options::record_runtime_stat)
        stat.nLineupSucc++;
      b.setLeaving();
      syncSignal(&b, true); // Signal all threads blocking on this barrier.
    } else {
      // NOP. There could be a case that after timeout happens,
      // all threads arrive, then we just let them do NOP, and deterministic.
    } 
  } else {
    if (b.isArriving()) {
      syncWait(&b, _S::getTurnCount() + b.timeout);
     
      // Handle timeout here, since the wait() would call getTurn and still grab the turn.
      if (b.nactive < b.count && b.isArriving()) {
        /*b.nTimeout++;
        fprintf(stderr, "lineupStart opaque_type %p, tid %d, timeout  (%ld:%ld)!\n",
          (void *)opaque_type, _S::self(), b.nSuccess, b.nTimeout);*/
        if (options::record_runtime_stat)
          stat.nLineupTimeout++;
        b.setLeaving();
        syncSignal(&b, true); // Signal all threads blocking on this barrier.
      }
    } else {
      // proceed. NOP.
    }
  }
   
  SCHED_TIMER_END(syncfunc::tern_lineup_start, (uint64_t)opaque_type);

  record_rdtsc_op(__FUNCTION__, "END", 1, NULL); // Record rdtsc start, disabled by default.
}

template <typename _S>
void RecorderRT<_S>::lineupEnd(long opaque_type) {
  unsigned ins = opaque_type;
  if (options::enforce_non_det_annotations && inNonDet) {
    if (options::record_runtime_stat)
      stat.nNonDetPthreadSync++;
    add_non_det_var((void *)opaque_type);
    return;
  }
  SCHED_TIMER_START;
  refcnt_bar_map::iterator bi = refcnt_bars.find(opaque_type);
  assert(bi != refcnt_bars.end() && "refcnt barrier is not initialized!");
  ref_cnt_barrier_t &b = bi->second;
  b.nactive--;
  //fprintf(stderr, "lineupEnd opaque_type %p, tid %d, nactive %u\n", (void *)opaque_type, _S::self(), b.nactive);
  if (b.nactive == 0 && b.isLeaving()) {
    b.setArriving();
  }
  SCHED_TIMER_END(syncfunc::tern_lineup_end, (uint64_t)opaque_type);
}

template <typename _S>
void RecorderRT<_S>::nonDetStart() {
  unsigned ins = 0;
  dprintf("nonDetStart, tid %d, self %u\n", _S::self(), (unsigned)pthread_self());
  SCHED_TIMER_START;
  if (options::record_runtime_stat)
    stat.nNonDetRegions++;

  nNonDetWait++;

  /** All non-det operations are blocked on this fake var until runq is empty, 
  i.e., all valid (except idle thread) xtern threads are paused.
  This wait works like a lineup with unlimited timeout, which is for 
  maximizing the non-det regions. **/
  /* Do not call the wait() here, but _S::wait(), because we do not want to 
  involved dbug_thread_waiting/active here. */
  _S::wait(&nonDetCV);

  nNonDetWait--;

  SCHED_TIMER_END(syncfunc::tern_non_det_start, 0);
  /** Reuse existing xtern API. Get turn, remove myself from runq, and then pass turn. This 
  operation is determinisitc since we get turn. **/
  _S::block(__PRETTY_FUNCTION__);
  dprintf("nonDetStart is done, tid %d, self %u, turnCount %u\n", _S::self(), (unsigned)pthread_self(), _S::turnCount);
  assert(!inNonDet);
  inNonDet = true;
}

template <typename _S>
void RecorderRT<_S>::nonDetEnd() {
  dprintf("nonDetEnd, tid %d, self %u\n", _S::self(), (unsigned)pthread_self());
  assert(options::enforce_non_det_annotations == 1);
  assert(inNonDet);
  inNonDet = false;
  /** At this moment current thread won't call any non-det sync op any more, so we 
  do not need to worry about the order between this non_det_end() and other non-det sync
  in other threads' non-det regions, so we do not need to call the wait(NON_DET_BLOCKED)
  as in non_det_start(). **/
  _S::wakeup(); /** Reuse existing xtern API. Put myself to wake-up queue, 
                            other threads grabbing the turn will put myself back to runq. This operation is non-
                            determinisit since we do not get turn, but it is fine, because there is already 
                            some non-det sync ops within the region already. Note that after this point, the 
                            status of the thread is still runnable. **/
}

template <typename _S>
void RecorderRT<_S>::nonDetBarrierEnd(int bar_id, int cnt) {
  dprintf("nonDetBarrierEnd, tid %d, self %u\n", _S::self(), (unsigned)pthread_self());
  assert(options::enforce_non_det_annotations == 1);
  assert(inNonDet);
  inNonDet = false;
  /** At this moment current thread won't call any non-det sync op any more, so we
  do not need to worry about the order between this non_det_end() and other non-det sync
  in other threads' non-det regions, so we do not need to call the wait(NON_DET_BLOCKED)
  as in non_det_start(). **/
  _S::wakeup(); /** Reuse existing xtern API. Put myself to wake-up queue,
                            other threads grabbing the turn will put myself back to runq. This operation is non-
                            determinisit since we do not get turn, but it is fine, because there is already
                            some non-det sync ops within the region already. Note that after this point, the
                            status of the thread is still runnable. **/
}

template <typename _S>
void RecorderRT<_S>::threadDetach() {
  //BLOCK_TIMER_START(pthread_detach, 0, 0, pthread_self());
  // Below code is just the same as BLOCK_TIMER_START, write in this form to pass compiler.
  if (_S::interProStart()) {
    _S::block(__PRETTY_FUNCTION__);
  }
  fprintf(stderr, "CRANE: pid %d tid %d pself %u is detached by %s() hint.\n", getpid(), _S::self(), PSELF, __FUNCTION__);
}

template <typename _S>
void RecorderRT<_S>::threadDisableSchedPaxos() {
  options::sched_with_paxos = 0;
  fprintf(stderr, "CRANE: pid %d tid %d pself %u by %s(): disable sched with paxos, because this process is not worker process.\n",
    getpid(), _S::self(), PSELF, __FUNCTION__);fflush(stderr);
}

template <typename _S>
void RecorderRT<_S>::setBaseTime(struct timespec *ts) {
  // Do not need to enforce any turn here.
  dprintf("setBaseTime, tid %d, base time %ld.%ld\n", _S::self(), (long)ts->tv_sec, (long)ts->tv_nsec);
  assert(ts);
  my_base_time.tv_sec = ts->tv_sec;
  my_base_time.tv_nsec = ts->tv_nsec;
}

template <typename _S>
void RecorderRT<_S>::symbolic(unsigned ins, int &error, void *addr,
                              int nbyte, const char *name){
  SCHED_TIMER_START;
  SCHED_TIMER_END(syncfunc::tern_symbolic, (uint64_t)addr, (uint64_t)nbyte);
}

//////////////////////////////////////////////////////////////////////////
// Partially specialize RecorderRT for scheduler RecordSerializer.  The
// RecordSerializer doesn't really care about the order of synchronization
// operations, as long as the log faithfully records the actual order that
// occurs.  Thus, we can simplify the implementation of pthread cond var
// methods for RecordSerializer.

/*
template <>
int RecorderRT<RecordSerializer>::wait(void *chan, unsigned timeout) {
  typedef RecordSerializer _S;
  _S::putTurn();
  sched_yield();
  _S::getTurn();
  return 0;
}

template <>
void RecorderRT<RecordSerializer>::signal(void *chan, bool all) {
}

template <>
int RecorderRT<RecordSerializer>::pthreadMutexTimedLock(unsigned ins, int &error, pthread_mutex_t *mu,
                                                        const struct timespec *abstime) {
  typedef RecordSerializer _S;

  if(abstime == NULL)
    return pthreadMutexLock(ins, error, mu);

  int ret;
  SCHED_TIMER_START;
  while((ret=pthread_mutex_trylock(mu))) {
    assert(ret==EBUSY && "failed sync calls are not yet supported!");

    wait(mu);

    struct timespec curtime;
    struct timeval curtimetv;
    gettimeofday(&curtimetv, NULL);
    curtime.tv_sec = curtimetv.tv_sec;
    curtime.tv_nsec = curtimetv.tv_usec * 1000;
    if(curtime.tv_sec > abstime->tv_sec
       || (curtime.tv_sec == abstime->tv_sec &&
           curtime.tv_nsec >= abstime->tv_nsec)) {
      ret = ETIMEDOUT;
      break;
    }
  }
  SCHED_TIMER_END(syncfunc::pthread_mutex_timedlock, (uint64_t)mu, (uint64_t)ret);
 
  return ret;
}

template <>
int RecorderRT<RecordSerializer>::semTimedWait(unsigned ins, int &error, sem_t *sem,
                                               const struct timespec *abstime) {
  typedef RecordSerializer _S;
  int saved_err = 0;

  if(abstime == NULL)
    return semWait(ins, error, sem);
  
  int ret;
  SCHED_TIMER_START;
  while((ret=sem_trywait(sem))) {
    assert(errno==EAGAIN && "failed sync calls are not yet supported!");
    //_S::putTurn();
    //sched_yield();
    //_S::getTurn();
    wait(sem);

    struct timespec curtime;
    struct timeval curtimetv;
    gettimeofday(&curtimetv, NULL);
    curtime.tv_sec = curtimetv.tv_sec;
    curtime.tv_nsec = curtimetv.tv_usec * 1000;
    if(curtime.tv_sec > abstime->tv_sec
       || (curtime.tv_sec == abstime->tv_sec &&
           curtime.tv_nsec >= abstime->tv_nsec)) {
      ret = -1;
      error = ETIMEDOUT;
      saved_err = ETIMEDOUT;
      break;
    }
  }
  SCHED_TIMER_END(syncfunc::sem_timedwait, (uint64_t)sem, (uint64_t)ret);
  
  errno = saved_err;
  return ret;
}
*/
/// NOTE: recording may be nondeterministic because the order of turns may
/// not be the order in which threads arrive or leave barrier_wait().  if
/// we have N concurrent barrier_wait() but the barrier count is smaller
/// than N, a thread may block in one run but return in another.  This
/// means, in replay, we should not call barrier_wait at all
template <>
int RecorderRT<RecordSerializer>::pthreadBarrierWait(unsigned ins, int &error, 
                                                     pthread_barrier_t *barrier) {
  typedef RecordSerializer _S;
  int ret = 0;
    
  SCHED_TIMER_START;
  SCHED_TIMER_FAKE_END(syncfunc::pthread_barrier_wait, (uint64_t)barrier, (uint64_t)ret);

  _S::putTurn();
  _S::getTurn();  //  more getTurn for consistent number of getTurn with RRSchedler
  _S::incTurnCount(__PRETTY_FUNCTION__);
  _S::putTurn();

  errno = error;
  ret = pthread_barrier_wait(barrier);
  error = errno;
  assert((!ret || ret==PTHREAD_BARRIER_SERIAL_THREAD)
         && "failed sync calls are not yet supported!");

  //timespec app_time = update_time();
  _S::getTurn();
  sched_time = update_time();
  SCHED_TIMER_END(syncfunc::pthread_barrier_wait, (uint64_t)barrier, (uint64_t)ret);

  return ret;
}

/// FCFS version of pthread_cond_wait is a lot simpler than RR version.
///
/// can use lock + cv, and since we don't force threads to get turns in
/// some order, getTurnWithoutLock() is just a noop, and we don't need to
/// replace cond_signal with cond_broadcast.
///
/// semaphore and flag approaches wouldn't make much sense.
///
/// TODO: can we use spinlock instead of schedulerLock?  probably not due
/// to cond_wait problem
///

/// Why do we use our own lock?  To avoid nondeterminism in logging
///
///   getTurn();
///   putTurn();
///   (1) pthread_cond_wait(&cv, &mu);
///                                     getTurn();
///                                     pthread_mutex_lock(&mu) (or trylock())
///                                     putTurn();
///                                     getTurn();
///                                     pthread_signal(&cv)
///                                     putTurn();
///   (2) pthread_cond_wait(&cv, &mu);
///

template <>
int RecorderRT<RecordSerializer>::pthreadCondWait(unsigned ins, int &error, 
                pthread_cond_t *cv, pthread_mutex_t *mu){
  typedef RecordSerializer _S;
      int ret;
  SCHED_TIMER_START;
  pthread_mutex_unlock(mu);

  SCHED_TIMER_FAKE_END(syncfunc::pthread_cond_wait, (uint64_t)cv, (uint64_t)mu);
  errno = error;
  pthread_cond_wait(cv, RecordSerializer::getLock());
  error = errno;
  sched_time = update_time();

  while((ret=pthread_mutex_trylock(mu))) {
    assert(ret==EBUSY && "failed sync calls are not yet supported!");
    syncWait(mu);
  }
  nturn = RecordSerializer::incTurnCount(__PRETTY_FUNCTION__);
  SCHED_TIMER_END(syncfunc::pthread_cond_wait, (uint64_t)cv, (uint64_t)mu);

  return 0;
}

template <>
int RecorderRT<RecordSerializer>::pthreadCondTimedWait(unsigned ins, int &error, 
    pthread_cond_t *cv, pthread_mutex_t *mu, const struct timespec *abstime){
  typedef RecordSerializer _S;
  SCHED_TIMER_START;
  pthread_mutex_unlock(mu);
  SCHED_TIMER_FAKE_END(syncfunc::pthread_cond_timedwait, (uint64_t)cv, (uint64_t)mu);

  errno = error;
  int ret = pthread_cond_timedwait(cv, _S::getLock(), abstime);
  error = errno;
  if(ret == ETIMEDOUT) {
    dprintf("%d timed out from timedwait\n", _S::self());
  }
  assert((ret==0||ret==ETIMEDOUT) && "failed sync calls are not yet supported!");

  pthreadMutexLockHelper(mu);
  SCHED_TIMER_END(syncfunc::pthread_cond_timedwait, (uint64_t)cv, (uint64_t)mu, (uint64_t) ret);
 
  return ret;
}

template <>
int RecorderRT<RecordSerializer>::pthreadCondSignal(unsigned ins, int &error, 
                                                 pthread_cond_t *cv){
  typedef RecordSerializer _S;

  SCHED_TIMER_START;
  errno = error;
  pthread_cond_signal(cv);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_cond_signal, (uint64_t)cv);

  return 0;
}

template <>
int RecorderRT<RecordSerializer>::pthreadCondBroadcast(unsigned ins, int &error, 
                                                    pthread_cond_t*cv){
  typedef RecordSerializer _S;

  SCHED_TIMER_START;
  errno = error;
  pthread_cond_broadcast(cv);
  error = errno;
  SCHED_TIMER_END(syncfunc::pthread_cond_broadcast, (uint64_t)cv);

  return 0;
}

template <typename _S>
bool RecorderRT<_S>::socketFd(int fd) {
  struct stat st;
  fstat(fd, &st);
  return ((st.st_mode & S_IFMT) == S_IFSOCK);
}

template <typename _S>
bool RecorderRT<_S>::regularFile(int fd) {
  struct stat st;
  fstat(fd, &st);
  // If it is neither a socket, nor a fifo, then it is regular file (not a inter-process communication media).
  return ((S_IFSOCK != (st.st_mode & S_IFMT)) && (S_IFIFO != (st.st_mode & S_IFMT)));
}

template <typename _S>
int RecorderRT<_S>::__accept(unsigned ins, int &error, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen)
{
  SOCKET_TIMER_DECL;
  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  if (options::sched_with_paxos == 0) {
    BLOCK_TIMER_START(accept, ins, error, sockfd, cliaddr, addrlen);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_ACCEPT, sockfd);
  }
  int ret = Runtime::__accept(ins, error, sockfd, cliaddr, addrlen);
  int from_port = 0;
  int to_port = 0;
  if (options::log_sync) {
    to_port = ((struct sockaddr_in *)cliaddr)->sin_port;
    struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);
    getsockname(sockfd, (struct sockaddr *)&servaddr, &len);
    from_port = servaddr.sin_port;
  }
  if (options::sched_with_paxos == 0) {
    BLOCK_TIMER_END(syncfunc::accept, (uint64_t)ret, (uint64_t)from_port, (uint64_t) to_port);
  } else {
    // this code should be put at after accept() is returned so that we can get the conns mapping.
#ifdef DEBUG_SCHED_WITH_PAXOS
    struct timeval tnow;
    gettimeofday(&tnow, NULL);
    debugpaxos( "Server thread pself %u tid %d accepted socket %d time <%ld.%ld>\n",
      (unsigned)pthread_self(), _S::self(), ret, tnow.tv_sec, tnow.tv_usec);
#endif
    assert (op.type == PAXQ_CONNECT && ret != -1);
    conns_add_pair(op.connection_id, ret);
    SOCKET_TIMER_END(syncfunc::accept, (uint64_t)ret, (uint64_t)from_port, (uint64_t) to_port);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__accept4(unsigned ins, int &error, int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen, int flags)
{
  SOCKET_TIMER_DECL;
  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  if (options::sched_with_paxos == 0) {
    BLOCK_TIMER_START(accept4, ins, error, sockfd, cliaddr, addrlen, flags);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_ACCEPT, sockfd);
  }
  int ret = Runtime::__accept4(ins, error, sockfd, cliaddr, addrlen, flags);
  if (options::sched_with_paxos == 0) {
    BLOCK_TIMER_END(syncfunc::accept4, (uint64_t) ret);
  } else {
    // this code should be put at after accept() is returned so that we can get the conns mapping.
    assert (op.type == PAXQ_CONNECT && ret != -1);
    conns_add_pair(op.connection_id, ret);
    SOCKET_TIMER_END(syncfunc::accept4, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__connect(unsigned ins, int &error, int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
  int ret = Runtime::__connect(ins, error, sockfd, serv_addr, addrlen);
  /*Don't need this code for now, because we do not need to schedule this operation.
  int from_port = 0;
  int to_port = 0;
  if (options::log_sync) {
    from_port = ((const struct sockaddr_in*) serv_addr)->sin_port;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    getsockname(sockfd, (struct sockaddr *)&cliaddr, &len);
    to_port = cliaddr.sin_port;
  }*/
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__send(unsigned ins, int &error, int sockfd, const void *buf, size_t len, int flags)
{
  int ret;
  if (options::light_log_sync == 1) {
    SCHED_TIMER_START;
    ret = Runtime::__send(ins, error, sockfd, buf, len, flags);
    _S::logNetworkOutput(_S::self(), __FUNCTION__, buf, len);
    SCHED_TIMER_END(syncfunc::send, (uint64_t) ret);
  } else
    ret = Runtime::__send(ins, error, sockfd, buf, len, flags);
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__sendto(unsigned ins, int &error, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
  int ret;
  if (options::light_log_sync == 1) {
    SCHED_TIMER_START;
    ret = Runtime::__sendto(ins, error, sockfd, buf, len, flags, dest_addr, addrlen);
    _S::logNetworkOutput(_S::self(), __FUNCTION__, buf, len);
    SCHED_TIMER_END(syncfunc::sendto, (uint64_t) ret);
  } else
    ret = Runtime::__sendto(ins, error, sockfd, buf, len, flags, dest_addr, addrlen);
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__sendmsg(unsigned ins, int &error, int sockfd, const struct msghdr *msg, int flags)
{
  int ret;
  if (options::light_log_sync == 1) {
    SCHED_TIMER_START;
    ret = Runtime::__sendmsg(ins, error, sockfd, msg, flags);
    _S::logNetworkOutput(_S::self(), __FUNCTION__, msg->msg_control, msg->msg_controllen);
    SCHED_TIMER_END(syncfunc::sendmsg, (uint64_t) ret);
  } else
    ret = Runtime::__sendmsg(ins, error, sockfd, msg, flags);
  return ret;
}

/*TODO: current implementation may have an atomicity problem:
before this ret is returned, the conn_id is not erased by client's close() yet,
however, after than, client's close() is executed and the recv() should be non 
det, but then the ret of this function may cause the runtime to still run into deterministic execution. */
bool clientSideClosed(int serverSockFd) {
  if (options::sched_with_paxos == 1) {
    paxq_lock();
    bool ret = !conns_conn_id_exist(serverSockFd);
    paxq_unlock();
    return ret;
  } else
    return false;
}

template <typename _S>
ssize_t RecorderRT<_S>::__recv(unsigned ins, int &error, int sockfd, void *buf, size_t len, int flags)
{
  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  SOCKET_TIMER_DECL;
  bool client_closed = clientSideClosed(sockfd);
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, sockfd);
    BLOCK_TIMER_START(recv, ins, error, sockfd, buf, len, flags);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, sockfd, NULL, (unsigned)len);
  }
  ssize_t ret = Runtime::__recv(ins, error, sockfd, buf, len, flags);
  debugpaxos("SOCK-RECV pself %u tid %d: %s(sock %d, bytes %u)\n", PSELF, _S::self(), __FUNCTION__, sockfd, (unsigned)ret);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::recv, (uint64_t) ret);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, ret);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::recv, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__recvfrom(unsigned ins, int &error, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  SOCKET_TIMER_DECL;
  bool client_closed = clientSideClosed(sockfd);
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, sockfd);
    BLOCK_TIMER_START(recvfrom, ins, error, sockfd, buf, len, flags, src_addr, addrlen);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, sockfd, NULL, (unsigned)len);
  }
  ssize_t ret = Runtime::__recvfrom(ins, error, sockfd, buf, len, flags, src_addr, addrlen);
  debugpaxos("SOCK-RECV pself %u tid %d: %s(sock %d, bytes %u)\n", PSELF, _S::self(), __FUNCTION__, sockfd, (unsigned)ret);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::recvfrom, (uint64_t) ret);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, ret);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::recvfrom, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__recvmsg(unsigned ins, int &error, int sockfd, struct msghdr *msg, int flags)
{
  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  SOCKET_TIMER_DECL;
  bool client_closed = clientSideClosed(sockfd);
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, sockfd);
    BLOCK_TIMER_START(recvmsg, ins, error, sockfd, msg, flags);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, sockfd, NULL, (unsigned)msg->msg_controllen);
  }
  ssize_t ret = Runtime::__recvmsg(ins, error, sockfd, msg, flags);
  debugpaxos("SOCK-RECV pself %u tid %d: %s(sock %d, bytes %u, expected %u)\n",
    PSELF, _S::self(), __FUNCTION__, sockfd, (unsigned)ret, (unsigned)msg->msg_controllen);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::recvmsg, (uint64_t) ret);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, ret);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::recvmsg, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__read(unsigned ins, int &error, int fd, void *buf, size_t count)
{
  // First, handle regular IO.
  if (options::RR_ignore_rw_regular_file && !socketFd(fd))
    return read(fd, buf, count);  // Directly call the libc read() for regular IO.

  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  SOCKET_TIMER_DECL;
  bool client_closed = clientSideClosed(fd);
  // Second, handle inter-process IO.
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, fd);
    BLOCK_TIMER_START(read, ins, error, fd, buf, count);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, fd, NULL, (unsigned)count);
  }
  ssize_t ret = Runtime::__read(ins, error, fd, buf, count);
  debugpaxos("SOCK-RECV pself %u tid %d: %s(sock %d, bytes %u)\n", PSELF, _S::self(), __FUNCTION__, fd, (unsigned)ret);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::read, (uint64_t) fd, (uint64_t) ret);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, ret);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::read, (uint64_t) fd, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__write(unsigned ins, int &error, int fd, const void *buf, size_t count)
{
  ssize_t ret;
  /* Heming: weird, have to comment out the second condition for apache light_log_sync. */
  if (options::light_log_sync == 1 && socketFd(fd)) {
    SCHED_TIMER_START;
    ret = Runtime::__write(ins, error, fd, buf, count);
    _S::logNetworkOutput(_S::self(), __FUNCTION__, buf, count);
    SCHED_TIMER_END(syncfunc::write, (uint64_t) ret);
    /*} else {
      fprintf(stderr, "Server application pid %d sends %u bytes to sockets or reg files (reg? %d).\n",
        getpid(), (unsigned)count, regularFile(fd));
      ret = Runtime::__write(ins, error, fd, buf, count);
    }*/
  } else
    ret = Runtime::__write(ins, error, fd, buf, count);
  return ret;
}

template <typename _S>
size_t RecorderRT<_S>::__fread(unsigned ins, int &error, void * ptr, size_t size, size_t count, FILE * stream)
{
  // First, handle regular IO.
  int fd = fileno(stream);
  if (options::RR_ignore_rw_regular_file && !socketFd(fd))
    return fread(ptr, size, count, stream);  // Directly call the libc fread() for regular IO.

  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  // Second, handle inter-process IO.
  SOCKET_TIMER_DECL;
  bool client_closed = clientSideClosed(fd);
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, fd);
    BLOCK_TIMER_START(fread, ins, error, ptr, size, count, stream);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, fd, NULL, (unsigned)size*count);
  }
  size_t ret = Runtime::__fread(ins, error, ptr, size, count, stream);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::fread, (uint64_t) ptr, (uint64_t) size);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, ret);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::fread, (uint64_t) ptr, (uint64_t) size);
  }
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__pread(unsigned ins, int &error, int fd, void *buf, size_t count, off_t offset)
{
  // First, handle regular IO.
  if (options::RR_ignore_rw_regular_file && !socketFd(fd))
    return pread(fd, buf, count, offset);  // Directly call the libc pread() for regular IO.

  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  // Second, handle inter-process IO.
  SOCKET_TIMER_DECL;
  bool client_closed = clientSideClosed(fd);
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, fd);
    BLOCK_TIMER_START(pread, ins, error, fd, buf, count, offset);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, fd, NULL, (unsigned)count);
  }
  ssize_t ret = Runtime::__pread(ins, error, fd, buf, count, offset);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::pread, (uint64_t) fd, (uint64_t) ret);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, ret);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::pread, (uint64_t) fd, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
ssize_t RecorderRT<_S>::__pwrite(unsigned ins, int &error, int fd, const void *buf, size_t count, off_t offset)
{
  ssize_t ret;
  if (options::light_log_sync == 1 && socketFd(fd)) {
    SCHED_TIMER_START;
    ret = Runtime::__pwrite(ins, error, fd, buf, count, offset);
    _S::logNetworkOutput(_S::self(), __FUNCTION__, buf, count);
    SCHED_TIMER_END(syncfunc::pwrite, (uint64_t) ret);
  } else
    ret = Runtime::__pwrite(ins, error, fd, buf, count, offset);
  return ret;
}

void* get_select_wait_obj(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {

  if (writefds) {
    if (readfds) {
      fprintf(stderr, "CRANE: server's select() writefds and readfds are not NULL. Disabled. Please contact developers\n");
      exit(1);
    } else {
      fprintf(stderr, "CRANE: server's select() only writefds is not NULL. This select() becomes non-det\n");
      return NULL;
    }
  }
  bool hasBindedSock = false;
  int num_server_sock = 0;
  void *ret = (void *)(long)conns_get_port_from_tid(getpid());
  for (int i = 0; i < nfds; ++i) {
    fprintf(stderr, "Pself %u %s checking select [%d]\n", PSELF, __FUNCTION__, i);
    // If there are established server sockets with the clients, then use this sock for wait obj for _S::wait(obj).
    if (FD_ISSET(i, readfds) && conns_is_binded_socket(i))
      hasBindedSock = true;
    if (FD_ISSET(i, readfds) && conns_exist_by_server_sock(i)) {
      debugpaxos( "Server's select() finds established socket pair <%lu, %d>.\n",
        conns_get_conn_id(i), i);
      num_server_sock++;
      ret = (void *)(long)i;
    }
  }
  if (num_server_sock > 1) {
    fprintf(stderr, "CRANE: server's select() num_server_sock > 1. Use the universal port as wait obj\n");
    assert(false);
    ret = (void *)(long)conns_get_port_from_tid(getpid());
  }
  fprintf(stderr, "Pself %u %s hasBindedSock %d, num_server_sock %d\n",
    (unsigned)pthread_self(), __FUNCTION__, hasBindedSock, num_server_sock);
  if (hasBindedSock == false && num_server_sock == 0)
    return NULL;
  else
    return ret;
}

template <typename _S>
int RecorderRT<_S>::__select(unsigned ins, int &error, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
  void *waitObj = NULL;
  if (options::sched_with_paxos == 1) {
    paxq_lock();
    waitObj = get_select_wait_obj(nfds, readfds, writefds, exceptfds);
    paxq_unlock();
  }

  int ret;
  SOCKET_TIMER_DECL;
  if (options::sched_with_paxos == 0 || waitObj == NULL) {
    BLOCK_TIMER_START(select, ins, error, nfds, readfds, writefds, exceptfds, timeout);
    ret = Runtime::__select(ins, error, nfds, readfds, writefds, exceptfds, timeout);
    BLOCK_TIMER_END(syncfunc::select, (uint64_t) ret);
  } else {
    SOCKET_TIMER_START;
    schedSocketOp(__FUNCTION__, DMT_SELECT, -1, waitObj);
    /* Because all incoming sockets are controled by us, make the timeout NULL to avoid nondeterminism.*/
    ret = Runtime::__select(ins, error, nfds, readfds, writefds, exceptfds, NULL);
    SOCKET_TIMER_END(syncfunc::select, (uint64_t) ret);
  }
  return ret;
}

void* get_epoll_wait_obj(int epfd, struct epoll_event *events, int maxevents) {
  bool hasBindedSock = false;
  int num_server_sock = 0;
  bool hasWrite = false;
  bool hasRead = false;
  for (int i = 0; i < maxevents; ++i) {
    if ((events[i].events & EPOLLOUT)) {
      hasWrite = true;
    }
    if ((events[i].events & EPOLLIN)) {
      hasRead = true;
    }
  }
  if (hasWrite) {
    if (hasRead) {
      fprintf(stderr, "CRANE: server's epoll_wait() have both write and read events. Disabled. Please contact developers\n");
      exit(1);
    } else {
      fprintf(stderr, "CRANE: server's epoll_wait() have only write events. This epoll_wait() becomes non-det\n");
      return NULL;
    }
  }
  
  void *ret = (void *)(long)conns_get_port_from_tid(getpid());
  for (int i = 0; i < maxevents; ++i) {
    // If there are established server sockets with the clients, then use this sock for wait obj for _S::wait(obj).
    int server_sock = events[i].data.fd;
    fprintf(stderr, "Pself %u %s checking epoll events(%p)[%d].fd %d\n",
      (unsigned)pthread_self(), __FUNCTION__, (void *)&events, i, server_sock);
    if (conns_is_binded_socket(server_sock))
      hasBindedSock = true;
    if (conns_exist_by_server_sock(server_sock)) {
      debugpaxos( "Server's epoll_wait() finds established socket pair <%lu, %d>.\n",
        conns_get_conn_id(server_sock), server_sock);
      num_server_sock++;
      ret = (void *)(long)server_sock;
    }
  }
  if (num_server_sock > 1) {
    fprintf(stderr, "CRANE: server's epoll_wait() num_server_sock > 1. Use the universal port as wait obj\n");
    assert(false);
    ret = (void *)(long)conns_get_port_from_tid(getpid());
  }
  fprintf(stderr, "Pself %u %s hasBindedSock %d, num_server_sock %d\n",
    (unsigned)pthread_self(), __FUNCTION__, hasBindedSock, num_server_sock);
  if (hasBindedSock == false && num_server_sock == 0)
    return NULL;
  else
    return ret;
}

template <typename _S>
int RecorderRT<_S>::__epoll_wait(unsigned ins, int &error, int epfd, struct epoll_event *events, int maxevents, int timeout)
{  
  void *waitObj = NULL;
  if (options::sched_with_paxos == 1) {
    paxq_lock();
    waitObj = get_epoll_wait_obj(epfd, events, maxevents);
    paxq_unlock();
  }

  int ret;
  SOCKET_TIMER_DECL;
  if (options::sched_with_paxos == 0 || waitObj == NULL) {
    BLOCK_TIMER_START(epoll_wait, ins, error, epfd, events, maxevents, timeout);
    ret = Runtime::__epoll_wait(ins, error, epfd, events, maxevents, timeout);
    BLOCK_TIMER_END(syncfunc::epoll_wait, (uint64_t) ret);
  } else {
    SOCKET_TIMER_START;
    schedSocketOp(__FUNCTION__, DMT_SELECT, -1);
    /* Because all incoming sockets are controled by us, make the timeout -1 to avoid nondeterminism.*/
    ret = Runtime::__epoll_wait(ins, error, epfd, events, maxevents, -1);
    SOCKET_TIMER_END(syncfunc::epoll_wait, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__epoll_create(unsigned ins, int &error, int size)
{  
  SOCKET_TIMER_DECL;
  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_START;
  }
  int ret = Runtime::__epoll_create(ins, error, size);
  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_END(syncfunc::epoll_create, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__epoll_ctl(unsigned ins, int &error, int epfd, int op, int fd, struct epoll_event *event)
{  
  SOCKET_TIMER_DECL;
  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_START;
  }
  int ret = Runtime::__epoll_ctl(ins, error, epfd, op, fd, event);
  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_END(syncfunc::epoll_ctl, (uint64_t) ret);
  }
  return ret;
}

void* get_poll_wait_obj(struct pollfd *fds, nfds_t nfds) {
  bool hasBindedSock = false;
  int num_server_sock = 0;
  bool hasWrite = false;
  bool hasRead = false;
  for (int i = 0; i < nfds; ++i) {
    if ((fds[i].events & POLLOUT) || (fds[i].events & POLLWRNORM) || (fds[i].events & POLLWRBAND)) {
      hasWrite = true;
    }
    if ((fds[i].events & POLLIN) || (fds[i].events & POLLRDNORM) || (fds[i].events & POLLRDBAND)) {
      hasRead = true;
    }
  }
  if (hasWrite) {
    if (hasRead) {
      fprintf(stderr, "CRANE: server's poll() have both write and read events. Disabled. Please contact developers\n");
      exit(1);
    } else {
      fprintf(stderr, "CRANE: server's poll() have only write events. This poll() becomes non-det\n");
      return NULL;
    }
  }
  
  void *ret = (void *)(long)conns_get_port_from_tid(getpid());
  for (int i = 0; i < nfds; ++i) {
    // If there are established server sockets with the clients, then use this sock for wait obj for _S::wait(obj).
    int server_sock = fds[i].fd;
    fprintf(stderr, "Pself %u %s checking poll fds(%p)[%d].fd %d\n",
      (unsigned)pthread_self(), __FUNCTION__, (void *)&fds, i, server_sock);
    if (conns_is_binded_socket(server_sock))
      hasBindedSock = true;
    if (conns_exist_by_server_sock(server_sock)) {
      debugpaxos( "Server's poll() finds established socket pair <%lu, %d>.\n",
        conns_get_conn_id(server_sock), server_sock);
      num_server_sock++;
      ret = (void *)(long)server_sock;
    }
  }
  if (num_server_sock > 1) {
    fprintf(stderr, "CRANE: server's poll() num_server_sock > 1. Use the universal port as wait obj\n");
    assert(false);
    ret = (void *)(long)conns_get_port_from_tid(getpid());
  }
  fprintf(stderr, "Pself %u %s hasBindedSock %d, num_server_sock %d\n",
    (unsigned)pthread_self(), __FUNCTION__, hasBindedSock, num_server_sock);
  if (hasBindedSock == false && num_server_sock == 0)
    return NULL;
  else
    return ret;
}

template <typename _S>
int RecorderRT<_S>::__poll(unsigned ins, int &error, struct pollfd *fds, nfds_t nfds, int timeout)
{
  void *waitObj = NULL;
  if (options::sched_with_paxos == 1) {
    paxq_lock();
    waitObj = get_poll_wait_obj(fds, nfds);
    paxq_unlock();
  }

  int ret;
  SOCKET_TIMER_DECL;
  if (options::sched_with_paxos == 0 || waitObj == NULL) {
    BLOCK_TIMER_START(poll, ins, error, fds, nfds, timeout);
    ret = Runtime::__poll(ins, error, fds, nfds, timeout);
    BLOCK_TIMER_END(syncfunc::poll, (uint64_t)fds, (uint64_t)nfds, (uint64_t)timeout, (uint64_t)ret);
  } else {
    SOCKET_TIMER_START;
    schedSocketOp(__FUNCTION__, DMT_SELECT, -1, waitObj);
    /* Because all incoming sockets are controled by us, make the timeout -1 to avoid nondeterminism.*/
    ret = Runtime::__poll(ins, error, fds, nfds, -1);
    SOCKET_TIMER_END(syncfunc::poll, (uint64_t)fds, (uint64_t)nfds, (uint64_t)timeout, (uint64_t)ret);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__bind(unsigned ins, int &error, int socket, const struct sockaddr *address, socklen_t address_len)
{
  SOCKET_TIMER_DECL;
  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_START;
    struct sockaddr_in *si = (sockaddr_in*)address;
    unsigned port = (unsigned)ntohs(si->sin_port);
    fprintf(stderr, "Server thread pself %u pid/tid %d binds port %u, socket %d\n",
      (unsigned)pthread_self(), getpid(), port, socket);
    conns_add_tid_port_pair(getpid(), port, socket);//Currenty use pid instead of _S::self().
  }
  int ret = Runtime::__bind(ins, error, socket, address, address_len);
  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_END(syncfunc::bind, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__sigwait(unsigned ins, int &error, const sigset_t *set, int *sig)
{
  BLOCK_TIMER_START(sigwait, ins, error, set, sig);
  int ret = Runtime::__sigwait(ins, error, set, sig);
  debugpaxos( "WARN: pid %d tid %d Crane has not supported deterministic %s() yet.\n", getpid(), _S::self(), __FUNCTION__);
  BLOCK_TIMER_END(syncfunc::sigwait, (uint64_t) ret);
  return ret;
}

template <typename _S>
char *RecorderRT<_S>::__fgets(unsigned ins, int &error, char *s, int size, FILE *stream)
{
  SOCKET_TIMER_DECL;
  // First, handle regular IO.
  int fd = fileno(stream);
  if (options::RR_ignore_rw_regular_file && !socketFd(fd))
    return fgets(s, size, stream);  // Directly call the libc fgets() for regular IO.

  paxos_op op = {0, 0, PAXQ_INVALID, 0};
  bool client_closed = clientSideClosed(fd);
  // Second, handle inter-process IO.
  if (options::sched_with_paxos == 0 || client_closed) {
    debugpaxos("NON DET: Pself %u tid %d: %s(sock %d)\n", PSELF, _S::self(), __FUNCTION__, fd);
    BLOCK_TIMER_START(fgets, ins, error, s, size, stream);
  } else {
    SOCKET_TIMER_START;
    op = schedSocketOp(__FUNCTION__, DMT_RECV, fd, NULL, (unsigned)size);
  }
  char * ret = Runtime::__fgets(ins, error, s, size, stream);
  if (options::sched_with_paxos == 0 || client_closed) {
    BLOCK_TIMER_END(syncfunc::fgets, (uint64_t) ret);
  } else {
    /** A recv() at server side may correspond to multiple send() at client side. **/
    assert(op.type == PAXQ_SEND);
    popSendOps(op.connection_id, size);
    paxq_unlock();// Because there may be multiple send() from proxy that corresond to this ::__recv(), we must release the lock here for atomicity.
    SOCKET_TIMER_END(syncfunc::fgets, (uint64_t) ret);
  }
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__kill(unsigned ins, int &error, pid_t pid, int sig)
{
  int ret = Runtime::__kill(ins, error, pid, sig);
  return ret;
}

template <typename _S>
pid_t RecorderRT<_S>::__fork(unsigned ins, int &error)
{
  dprintf("pid %d enters fork\n", getpid());
  pid_t ret;

  if (options::log_sync)
    Logger::the->flush(); // so child process won't write it again

  /* Although this is inter-process operation, and we need to involve dbug
    tool (debug needs to register/unregister threads based on fork()), we do
    not need to switch from SCHED_TIMER_START to BLOCK_TIMER_START, 
    because at this moment the child thread is not alive yet, so dbug could not 
    enforce any order between child and parent processes. And, we need this
    sched_* scheduling way to update the runq and waitq of parent
    and child processes safely. */
  SCHED_TIMER_START;
  ret = Runtime::__fork(ins, error);
  if(ret == 0) {
    // child process returns from fork; re-initializes scheduler and logger state
    Logger::threadEnd(); // close log
    Logger::threadBegin(_S::self()); // re-open log
    assert(!sem_init(&thread_begin_sem, 0, 0));
    assert(!sem_init(&thread_begin_done_sem, 0, 0));
    _S::childForkReturn();
  } else
    assert(ret > 0);
  SCHED_TIMER_END(syncfunc::fork, (uint64_t) ret);

  // FIXME: this is gross.  idle thread should be part of RecorderRT
  if (ret == 0 && options::launch_idle_thread) {
    Space::exitSys();
    pthread_cond_init(&idle_cond, NULL);
    pthread_mutex_init(&idle_mutex, NULL);
    int res = tern_pthread_create(0xdead0000, &idle_th, NULL, idle_thread, NULL);
    assert(res == 0 && "tern_pthread_create failed!");
    Space::enterSys();
  }

  dprintf("pid %d leaves fork\n", getpid());
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__execv(unsigned ins, int &error, const char *path, char *const argv[])
{
  if (options::log_sync)
    Logger::the->flush(); // so child process won't write it again
    
  int ret = 0;
  SCHED_TIMER_START;
  nturn = 0; // Just avoid compiler warning.
  ret = Runtime::__execv(ins, error, path, argv);
  assert(false && "execv failed.");

  return ret;
}

template <typename _S>
pid_t RecorderRT<_S>::__wait(unsigned ins, int &error, int *status)
{
  BLOCK_TIMER_START(wait, ins, error, status);
  pid_t ret = Runtime::__wait(ins, error, status);
  debugpaxos( "WARN: pid %d tid %d Crane has not supported deterministic %s() yet.\n", getpid(), _S::self(), __FUNCTION__);
  BLOCK_TIMER_END(syncfunc::wait, (uint64_t)*status, (uint64_t)ret);
  return ret;
}

template <typename _S>
pid_t RecorderRT<_S>::__waitpid(unsigned ins, int &error, pid_t pid, int *status, int options)
{
  BLOCK_TIMER_START(waitpid, ins, error, pid, status, options);
  pid_t ret = Runtime::__waitpid(ins, error, pid, status, options);
  debugpaxos( "WARN: pid %d tid %d Crane has not supported deterministic %s() yet.\n", getpid(), _S::self(), __FUNCTION__);
  BLOCK_TIMER_END(syncfunc::waitpid, (uint64_t)pid, (uint64_t)*status, (uint64_t)options, (uint64_t)ret);
  return ret;
}


template <typename _S>
int RecorderRT<_S>::schedYield(unsigned ins, int &error)
{
  int ret;
  if (options::enforce_non_det_annotations && inNonDet) {
    // Do not need to count nNonDetPthreadSync for this op.
    //fprintf(stderr, "non-det yield start tid %d...\n", _S::self());  
    ret = Runtime::__sched_yield(ins, error);
    //fprintf(stderr, "non-det yield end tid %d...\n", _S::self());  
    return ret;
  }
  SCHED_TIMER_START;
  ret = sched_yield();
  SCHED_TIMER_END(syncfunc::sched_yield, (uint64_t)ret);
  return ret;
}

// TODO: right now we treat sleep functions just as a turn; should convert
// real time to logical time
template <typename _S>
unsigned int RecorderRT<_S>::__sleep(unsigned ins, int &error, unsigned int seconds)
{
  struct timespec ts = {seconds, 0};
  SCHED_TIMER_START;
  // must call _S::getTurnCount with turn held
  unsigned timeout = _S::getTurnCount() + relTimeToTurn(&ts);
  _S::wait(NULL, timeout);
  SCHED_TIMER_END(syncfunc::sleep, (uint64_t) seconds * 1000000000);
  if (options::exec_sleep)
    ::sleep(seconds);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::__usleep(unsigned ins, int &error, useconds_t usec)
{
  struct timespec ts = {0, 1000*usec};
  SCHED_TIMER_START;
  // must call _S::getTurnCount with turn held
  unsigned timeout = _S::getTurnCount() + relTimeToTurn(&ts);
  _S::wait(NULL, timeout);
  SCHED_TIMER_END(syncfunc::usleep, (uint64_t) usec * 1000);
  if (options::exec_sleep)
    ::usleep(usec);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::__nanosleep(unsigned ins, int &error, 
                              const struct timespec *req,
                              struct timespec *rem)
{
 SCHED_TIMER_START;
   // must call _S::getTurnCount with turn held
  unsigned timeout = _S::getTurnCount() + relTimeToTurn(req);
  _S::wait(NULL, timeout);
  uint64_t nsec = !req ? 0 : (req->tv_sec * 1000000000 + req->tv_nsec); 
  SCHED_TIMER_END(syncfunc::nanosleep, (uint64_t) nsec);
  if (options::exec_sleep)
    ::nanosleep(req, rem);
  return 0;
}

template <typename _S>
int RecorderRT<_S>::__socket(unsigned ins, int &error, int domain, int type, int protocol)
{
  int ret = Runtime::__socket(ins, error, domain, type, protocol);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__listen(unsigned ins, int &error, int sockfd, int backlog)
{
  int ret = Runtime::__listen(ins, error, sockfd, backlog);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__shutdown(unsigned ins, int &error, int sockfd, int how)
{
  int ret = Runtime::__shutdown(ins, error, sockfd, how);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__getpeername(unsigned ins, int &error, int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return Runtime::__getpeername(ins, error, sockfd, addr, addrlen);
}

template <typename _S>
int RecorderRT<_S>::__getsockopt(unsigned ins, int &error, int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen)
{
  int ret = Runtime::__getsockopt(ins, error, sockfd, level, optname, optval, optlen);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__setsockopt(unsigned ins, int &error, int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
  int ret = Runtime::__setsockopt(ins, error, sockfd, level, optname, optval, optlen);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__pipe(unsigned ins, int &error, int pipefd[2])
{
  debugpaxos( "\n\nPARROT WARN: pipes are not fully supported yet!\n\n");
  int ret = Runtime::__pipe(ins, error, pipefd);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__fcntl(unsigned ins, int &error, int fd, int cmd, void *arg)
{
  int ret = Runtime::__fcntl(ins, error, fd, cmd, arg);
  return ret;
}


template <typename _S>
int RecorderRT<_S>::__close(unsigned ins, int &error, int fd)
{
  int ret;
  SOCKET_TIMER_DECL;
  if (options::RR_ignore_rw_regular_file && !socketFd(fd))
    return ret = Runtime::__close(ins, error, fd);

  if (options::sched_with_paxos == 1) {
    SOCKET_TIMER_START;
    paxq_print();
    conns_print();
    if (conns_exist_by_server_sock(fd)) {
      uint64_t conn_id = conns_get_conn_id(fd);
      debugpaxos( "Pself %u tid %d calls close(%d) with connection id %lu.\n",
        (unsigned)pthread_self(), _S::self(), fd, (unsigned long)conn_id);
      conns_erase_by_server_sock(fd);
    } else {
      fprintf(stderr, "Pself %u tid %d calls close(%d) with no connection id. Should be an outgoing socket, fine.\n",
        (unsigned)pthread_self(), _S::self(), fd);
    }
  }
  ret = Runtime::__close(ins, error, fd);
  if (options::record_runtime_stat)
    stat.print();  
  if (options::sched_with_paxos == 1) {
    struct timeval tnow;
    gettimeofday(&tnow, NULL);
    debugpaxos( "Server thread pself %u tid %d closed socket %d time <%ld.%ld>\n",
      (unsigned)pthread_self(), _S::self(), ret, tnow.tv_sec, tnow.tv_usec);
    SOCKET_TIMER_END(syncfunc::close, (uint64_t) ret);
  }  
  return ret;
}

template <>
unsigned int RecorderRT<RecordSerializer>::__sleep(unsigned ins, int &error, unsigned int seconds)
{
  typedef Runtime _P;
  return _P::__sleep(ins, error, seconds);
}

template <>
int RecorderRT<RecordSerializer>::__usleep(unsigned ins, int &error, useconds_t usec)
{
  typedef Runtime _P;
  return _P::__usleep(ins, error, usec);
}

template <>
int RecorderRT<RecordSerializer>::__nanosleep(unsigned ins, int &error, 
                                            const struct timespec *req,
                                            struct timespec *rem)
{
  typedef Runtime _P;
  return _P::__nanosleep(ins, error, req, rem);
}

template <typename _S>
time_t RecorderRT<_S>::__time(unsigned ins, int &error, time_t *t)
{
  return Runtime::__time(ins, error, t);
}

template <typename _S>
int RecorderRT<_S>::__clock_getres(unsigned ins, int &error, clockid_t clk_id, struct timespec *res)
{
  return Runtime::__clock_getres(ins, error, clk_id, res);
}

template <typename _S>
int RecorderRT<_S>::__clock_gettime(unsigned ins, int &error, clockid_t clk_id, struct timespec *tp)
{
  return Runtime::__clock_gettime(ins, error, clk_id, tp);
}

template <typename _S>
int RecorderRT<_S>::__clock_settime(unsigned ins, int &error, clockid_t clk_id, const struct timespec *tp)
{
  return Runtime::__clock_settime(ins, error, clk_id, tp);
}

template <typename _S>
int RecorderRT<_S>::__gettimeofday(unsigned ins, int &error, struct timeval *tv, struct timezone *tz)
{
  return Runtime::__gettimeofday(ins, error, tv, tz);
}

template <typename _S>
int RecorderRT<_S>::__settimeofday(unsigned ins, int &error, const struct timeval *tv, const struct timezone *tz)
{
  return Runtime::__settimeofday(ins, error, tv, tz);
}

template <typename _S>
struct hostent *RecorderRT<_S>::__gethostbyname(unsigned ins, int &error, const char *name)
{
  struct hostent *ret = Runtime::__gethostbyname(ins, error, name);
  return ret;
}

template <typename _S>
int RecorderRT<_S>::__gethostbyname_r(unsigned ins, int &error, const char *name, struct hostent *ret,
  char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
  int ret2 = Runtime::__gethostbyname_r(ins, error, name, ret, buf, buflen, result, h_errnop);
  return ret2;
}

template <typename _S>
int RecorderRT<_S>::__getaddrinfo(unsigned ins, int &error, const char *node, const char *service, const struct addrinfo *hints,
struct addrinfo **res)
{
  int ret2 = Runtime::__getaddrinfo(ins, error, node, service, hints, res);
  return ret2;
}

template <typename _S>
void RecorderRT<_S>::__freeaddrinfo(unsigned ins, int &error, struct addrinfo *res)
{
  Runtime::__freeaddrinfo(ins, error, res);
}

template <typename _S>
struct hostent *RecorderRT<_S>::__gethostbyaddr(unsigned ins, int &error, const void *addr, int len, int type)
{
  struct hostent *ret = Runtime::__gethostbyaddr(ins, error, addr, len, type);
  return ret;
}

template <typename _S>
char *RecorderRT<_S>::__inet_ntoa(unsigned ins, int &error, struct in_addr in) {
  char * ret = Runtime::__inet_ntoa(ins, error, in);
  return ret;
}

template <typename _S>
char *RecorderRT<_S>::__strtok(unsigned ins, int &error, char * str, const char * delimiters) {
  char * ret = Runtime::__strtok(ins, error, str, delimiters);
  return ret;
}

void popSendOps(uint64_t conn_id, unsigned recv_len) { 
  unsigned num_popped = 0;
  unsigned nbytes_recv = 0;
  const uint64_t delete_conn_id = (uint64_t)-1;
  assert(paxq_size() > 0 && recv_len != 0);
  paxos_op op = paxq_get_op(0);
  assert(op.connection_id == conn_id && op.type == PAXQ_SEND);
  debugpaxos("popSendOps start: conn_id %lu, recv_len %u, head op (%lu, %d)\n",
    (unsigned long)conn_id, recv_len, (unsigned long)op.connection_id, op.value);
  for (size_t i = 0; i < paxq_size(); i++) {// First pass, go over the whole pax queue and mark the deleted ones.
    op = paxq_get_op(i);
    if(op.connection_id == conn_id && op.type == PAXQ_SEND) {
      if (nbytes_recv + op.value > recv_len) // If the server's recv() buffer is full, just stop.
        break;
      num_popped++;
      nbytes_recv += op.value;
      debugpaxos("op deleted send value %d\n", op.value);
      paxq_set_conn_id(i, delete_conn_id); // Mark as deleted of the i th element.
    }
  }
  paxq_delete_ops(delete_conn_id, num_popped);
  debugpaxos("popSendOps() pself %u, recv(up to %u bytes), popped %u PAXQ_SEND operations for connection %lu, actual recv %u bytes\n",
    PSELF, recv_len, num_popped, (unsigned long)conn_id, nbytes_recv);
}

#if 1
static int curTimeBubbleCnt = 0;
template <typename _S>
paxos_op RecorderRT<_S>::schedSocketOp(const char *funcName, SyncType syncType, long sockFd,
  void *selectWaitObj, unsigned recvLen) {
  struct timeval tnow;
#ifdef DEBUG_SCHED_WITH_PAXOS
  if (_S::self() >= 2) { 
    gettimeofday(&tnow, NULL);
    fprintf(stderr, "ENTER schedSocketOp <%ld.%ld> pself %u tid %d, turnCount %u (%s, %s, %ld)\n",
      tnow.tv_sec, tnow.tv_usec, PSELF, _S::self(), _S::turnCount, funcName, charSyncType[syncType], sockFd);
  }
  if (syncType != DMT_REG_SYNC) {
    assert(sockFd > 0);
    paxq_lock();
    conns_print();
    paxq_print();
    paxq_unlock();
  }
#endif

  paxos_op op = {0, 0, PAXQ_INVALID, 0}; /** head of the paxos operation queue. **/
  paxq_lock();
  if (syncType == DMT_REG_SYNC) {
    while (paxq_size() == 0 || (paxq_get_op2(0, &op) && op.value < 0)) {
      if (paxq_size() == 0 && paxq_role_is_leader()) { // Invoke a timebubble.
        if (curTimeBubbleCnt == 0)
          curTimeBubbleCnt = options::sched_with_paxos_max; // Init.
        if (conns_get_conn_id_num() == 0) // This if check is just a performance hint.
          curTimeBubbleCnt = std::min(curTimeBubbleCnt+1, options::sched_with_paxos_max);
        else
          curTimeBubbleCnt = options::sched_with_paxos_min;
        paxq_notify_proxy(curTimeBubbleCnt);/* Just send a timebubble req. The 
                  timebubble operation will be inserted after each proxy receives the 
                  consensus. This is for atomicy (the paxq queue order is the 
                  same as the consensus order). */
      }
 
      paxq_unlock();
      debugpaxos("DMT sleep %d...\n", options::sched_with_paxos_usleep);
      ::usleep(options::sched_with_paxos_usleep);
      paxq_lock();
    }

    assert (paxq_size() > 0 && (paxq_get_op2(0, &op) && op.value >= 0));
    if (op.type == PAXQ_NOP) {
      if (op.value == 1) {
        paxq_pop_front(5);
      } else {
        int clk = paxq_dec_front_value();
        debugpaxos("Pself %u tid %d, turnCount %u, dec PAXQ_NOP clock: %d\n",
          PSELF, _S::self(), _S::turnCount, clk);
      }
    } else {
      if (op.type == PAXQ_CONNECT) {
        unsigned port = conns_get_port_from_tid(getpid()); 
        if (port == 0)
          port = DEFAULT_PORT;
        _S::signal((void *)(long)port);
      } else if (op.type == PAXQ_SEND) {
        _S::signal((void *)(long)conns_get_server_sock(op.connection_id)); 
      } else if (op.type == PAXQ_CLOSE) {
        _S::signal((void *)(long)conns_get_server_sock(op.connection_id));
        assert(conns_exist_by_conn_id(op.connection_id));
        conns_erase_by_conn_id(op.connection_id);
        paxq_pop_front(2);
      }
    }

  } else { /** if this is a socket operation **/
    paxq_unlock();
    if (syncType == DMT_SELECT) {
      assert(selectWaitObj); 
      _S::wait(selectWaitObj);
    } else if (syncType == DMT_ACCEPT) {
      unsigned port = conns_get_port_from_tid(getpid()); 
      if (port == 0)
        port = DEFAULT_PORT;
      _S::wait((void *)(long)port);
    } else {
      assert(syncType == DMT_RECV);
      _S::wait((void *)(long)sockFd);
    }
    paxq_lock();

    // When current thread reaches here, the head timebubble must be a socket op (see _S::signal() above).
    if (paxq_size() > 0) {
      if (syncType == DMT_SELECT) {
        goto algo_exit;// NOP.
      } else {
        op = paxq_get_op(0);
        if (syncType == DMT_RECV && op.type == PAXQ_SEND) {
          // For recv(), to ensure atomicy, we must check the actual bytes received and then unlock.
          goto algo_exit_without_unlock;
        } else if (syncType == DMT_ACCEPT && op.type == PAXQ_CONNECT) {
          paxq_pop_front(4);
        } else {
          assert(false && "CRANE: thread is waken up from unknown socket operation.");
        }
      } 
    }
  } /** end of handling socket operation. **/
 

algo_exit:
  paxq_unlock();

algo_exit_without_unlock:

#ifdef DEBUG_SCHED_WITH_PAXOS 
  if (_S::self() >= 2) {
    gettimeofday(&tnow, NULL);
    fprintf(stderr, "LEAVE schedSocketOp <%ld.%ld> pself %u tid %d, turnCount %u (%s, %s, %ld)\n",
      tnow.tv_sec, tnow.tv_usec, PSELF, _S::self(), _S::turnCount, funcName, charSyncType[syncType], sockFd);
  }
#endif

  return op;
}
#endif



#if 0
/* A paxos operation queue from the ab client.
paxq_print [0]: (1425233714, 0, CONNECT)
paxq_print [1]: (70144710450, 0, CONNECT)
paxq_print [2]: (1425233714, 1, SEND)
paxq_print [3]: (1425233714, 2, SEND)
paxq_print [4]: (70144710450, 1, SEND)
paxq_print [5]: (70144710450, 2, SEND)
paxq_print [6]: (1425233714, 3, CLOSE)
paxq_print [7]: (70144710450, 3, CLOSE)
*/
static bool hasAskClks = false; // This flag is to avoid asking proxy for clocks too frequently.
template <typename _S>
paxos_op RecorderRT<_S>::schedSocketOp(const char *funcName, SyncType syncType, long sockFd,
  void *selectWaitObj, unsigned recvLen) {
  // Debug info.
  assert(options::sched_with_paxos == 1);
#ifdef DEBUG_SCHED_WITH_PAXOS
  struct timeval tnow;
  if (_S::self() >= 2) { 
    gettimeofday(&tnow, NULL);
    fprintf(stderr, "ENTER schedSocketOp <%ld.%ld> pself %u tid %d, turnCount %u (%s, %s, %ld)\n",
      tnow.tv_sec, tnow.tv_usec, PSELF, _S::self(), _S::turnCount, funcName, charSyncType[syncType], sockFd);
  }
  if (syncType != DMT_REG_SYNC) {
    assert(sockFd > 0);
    paxq_lock();
    conns_print();
    paxq_print();
    paxq_unlock();
  }
#endif

  paxos_op op = {0, 0, PAXQ_INVALID, 0}; /** head of the paxos operation queue. **/
  paxq_lock();
  if (syncType == DMT_REG_SYNC) {
    int loopCnt = 0;
    while (conns_get_conn_id_num() > 0) {
      if (paxq_size() > 0) { // If has paxos op, check whether it is PAXQ_NOP.
        op = paxq_get_op(0);
        if (op.type != PAXQ_NOP) // If it is not PAXQ_NOP, just move forward to signal the right thread to process it.
          break;
        else { // If it is PAXQ_NOP
          if (op.value > 0) { //If the proxy has given DMT clocks.
            int clk = 0;
            if (op.value == 1) {
              paxq_pop_front(5);
              hasAskClks = false;
            } else
              clk = paxq_dec_front_value();
            /** Now current thread has "consumed and poped one socket operation",
            current thread can just leave this function and call the actual sync operation. **/
            debugpaxos("Pself %u tid %d, turnCount %u, dec PAXQ_NOP clock: %d\n", PSELF, _S::self(), _S::turnCount, clk);
            goto algo_exit;
          }
        }
      }

      assert(paxq_size() == 0 || (op = paxq_get_op(0) && op.type == PAXQ_NOP && op.value < 0));
      /** Busy wait, and sends a request (signal) to proxy for more clocks. **/
      paxq_unlock();
      ::usleep(100);
      loopCnt++;
      debugpaxos( "Server pself %u tid %d schedSocketOp(%s, %ld) BUSY WAITS, loopCnt %d\n",
        PSELF, _S::self(), charSyncType[syncType], sockFd, loopCnt);
      paxq_lock();
      if ((loopCnt > 20 && !hasAskClks) || loopCnt > 10000) { // Just some temporary parameters for performance.
        loopCnt = 0;
        if (paxq_role_is_leader()) { // If current node is leader.
          if (paxq_size() == 0) {
            paxq_insert_front(0/*Lock is already held*/, 0, 0, PAXQ_NOP,
              -1*options::sched_with_paxos_min*conns_get_conn_id_num());// Negative. Proxy will make it positive.
            debugpaxos( "Server pself %u tid %d schedSocketOp(%s, %ld) asks proxy for clks, loopCnt %d\n",
              PSELF, _S::self(), charSyncType[syncType], sockFd, loopCnt);
          }
          paxq_notify_proxy();
          hasAskClks = true;
        }
      }
    }

    assert(paxq_size() > 0 || conns_get_conn_id_num() == 0);
    if (paxq_size() > 0) {
      op = paxq_get_op(0);
      assert(op.type != PAXQ_NOP);
      /** No matter what type (CONNECT, SEND, or CLOSE) from the paxos queue, 
      we must wake up threads waiting on the port, because a server thread may wait
      on select() or accept() to wait for CONNECT, or wait on select()/poll() 
      to wait for SEND or CLOSE. **/
      if (op.type == PAXQ_CONNECT) {
        _S::signal((void *)(long)conns_get_port_from_tid(getpid()));
      } else if (op.type == PAXQ_SEND) { /** If current paxos op is SEND, then wake up the thread that waits on recv(). **/
        _S::signal((void *)(long)conns_get_server_sock(op.connection_id)); 
      } else if (op.type == PAXQ_CLOSE) {
        _S::signal((void *)(long)conns_get_server_sock(op.connection_id)); /** A server thread may wait on recv() or select() that may be triggered by this close() from client. **/
        assert(conns_exist_by_conn_id(op.connection_id));
        conns_erase_by_conn_id(op.connection_id);
        paxq_pop_front(2);
      }
    }
  } else {/** This is a blocking socket operation. **/
    while (true) {
      paxq_unlock();
      if (syncType == DMT_SELECT) {
        assert(selectWaitObj); 
        _S::wait(selectWaitObj); /** This selectWaitObj is either the port for listener thread or the sockfd for worker threads. **/
      } else if (syncType == DMT_ACCEPT) {
        unsigned port = conns_get_port_from_tid(getpid()); 
        _S::wait((void *)(long)port);
      } else {
        assert(syncType == DMT_RECV);
        _S::wait((void *)(long)sockFd);
      }
      paxq_lock();

      if (paxq_size() > 0) {
        if (syncType == DMT_SELECT) {
          /* If current thread is doing a select(), then we just need to break from
          the loop without popping the paxos operation, because at the moment of select()
          we can not know which operation triggers the select(), so our algorithm let the server's
          operation following the select() to pop the paxos operation (see above code). */
          break;
        } else {
          op = paxq_get_op(0);
          if (syncType == DMT_RECV && op.type == PAXQ_SEND) {
            /** We must exit with the lock held so that the actual ::__recv() is atomic with the PAXQ_SEND
            on proxy side. We will do the paxq_unlock() after the actual ::__recv() returns. **/
            goto algo_exit_without_unlock;
          } else if (syncType == DMT_ACCEPT && op.type == PAXQ_CONNECT) {
            paxq_pop_front(4);
            break;
          } else { /* Current server thread's socket operation does not match the paxos head
            operation, just go back to _S::wait(). */
            debugpaxos("Server thread pself %u tid %d schedSocketOp(syncType %s, sockFd %ld) \
              does not match paxos op type %d, goes back to _S::wait().\n",
              (unsigned)pthread_self(), _S::self(), charSyncType[syncType], sockFd, (int)op.type);
          }
        } 
      }
    }
  }


algo_exit:
  paxq_unlock();

algo_exit_without_unlock:

#ifdef DEBUG_SCHED_WITH_PAXOS
  if (_S::self() >= 2) {
    gettimeofday(&tnow, NULL);
    fprintf(stderr, "LEAVE schedSocketOp <%ld.%ld> pself %u tid %d, turnCount %u (%s, %s, %ld)\n",
      tnow.tv_sec, tnow.tv_usec, PSELF, _S::self(), _S::turnCount, funcName, charSyncType[syncType], sockFd);
  }
#endif

  return op;
}
#endif

} // namespace tern
