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
#ifndef __TERN_COMMON_RUNTIME_SCHED_H
#define __TERN_COMMON_RUNTIME_SCHED_H

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <list>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "run-queue.h"
#include "non-det-thread-set.h"

extern "C" {
extern int idle_done;
}

namespace tern {


/// assign an internal tern tid to each pthread tid; also maintains the
/// reverse map from pthread tid to tern tid.  This class itself doesn't
/// synchronize its methods; instead, the callers of these methods must
/// ensure that the methods are synchronized.
struct TidMap {
  enum {MainThreadTid = 0, IdleThreadTid = 1, InvalidTid = -1};

  typedef std::tr1::unordered_map<pthread_t, int> pthread_to_tern_map;
  typedef std::tr1::unordered_map<int, pthread_t> tern_to_pthread_map;
  typedef std::tr1::unordered_set<pthread_t>      pthread_tid_set;

  /// create a new tern tid and map pthread_tid to this new id
  int create(pthread_t pthread_th) {
    pthread_to_tern_map::iterator it = p_t_map.find(pthread_th);
    assert(it==p_t_map.end() && "pthread tid already in map!");
    p_t_map[pthread_th] = nthread;
    t_p_map[nthread] = pthread_th;
    return nthread++;
  }

  /// sets thread-local tern tid to be the tid of @self_th
  void self(pthread_t self_th) {
    pthread_to_tern_map::iterator it = p_t_map.find(self_th);
    if (it==p_t_map.end())
      fprintf(stderr, "pthread tid not in map!\n");
    assert(it!=p_t_map.end() && "pthread tid not in map!");
    self_tid = it->second;
  }

  /// remove thread @tern_tid from the maps and insert it into the zombie set
  void zombify(pthread_t self_th) {
    tern_to_pthread_map::iterator it = t_p_map.find(self());
    assert(it!=t_p_map.end() && "tern tid not in map!");
    assert(self_th==it->second && "mismatch between pthread tid and tern tid!");
    zombies.insert(it->second);
    p_t_map.erase(it->second);
    t_p_map.erase(it);
  }

  /// remove thread @pthread_th from the maps
  void reap(pthread_t pthread_th) {
    zombies.erase(pthread_th);
  }

  /// return tern tid of thread @pthread_th
  int getTid(pthread_t pthread_th) {
    pthread_to_tern_map::iterator it = p_t_map.find(pthread_th);
    if(it!=p_t_map.end())
      return it->second;
    return InvalidTid;
  }

  /// return pthread id given a parrot tid.
  pthread_t getPthreadTid(int tid) {
    tern_to_pthread_map::iterator it = t_p_map.find(tid);
    if(it!=t_p_map.end())
      return it->second;
    return (pthread_t)InvalidTid;
  }

  /// return if thread @pthread_th is in the zombie set
  bool zombie(pthread_t pthread_th) {
    pthread_tid_set::iterator it = zombies.find(pthread_th);
    return it!=zombies.end();
  }

  /// tern tid for current thread
  static int self() { return self_tid; }
  static __thread int self_tid;

  TidMap(pthread_t main_th) { init(main_th); }

  /// initialize state
  void init(pthread_t main_th) {
    nthread = 0;
    // add tid mappings for main thread
    create(main_th);
    // initialize self_tid for main thread in case the derived class
    // constructors call self().  The main thread may call
    // @self(pthread_self()) again to set @self_tid, but this assignment
    // is idempotent, so it doesn't matter
    self(main_th);
  }

protected:

  /// reset internal state to initial state
  void reset(pthread_t main_th) {
    p_t_map.clear();
    t_p_map.clear();
    zombies.clear();

    init(main_th);
  }

  pthread_to_tern_map p_t_map;
  tern_to_pthread_map t_p_map;
  pthread_tid_set     zombies;
  int nthread;
};

/// @Serializer defines the interface for a serializer that ensures that
/// all synchronizations are serialized.  A serializer doesn't attempt to
/// schedule them.  The nondeterministic recorder runtime and the replay
/// runtime use serializers, instead of schedulers.
struct Serializer: public TidMap {

  enum {FOREVER = UINT_MAX}; // wait forever w/o timeout

  /// wait on @chan until another thread calls signal(@chan), or turnCount
  /// is greater than or equal to @timeout if @timeout is not 0.  give up
  /// turn and re-grab it upon return.
  ///
  /// To avoid @chan conflicts, should choose @chan values from the same
  /// domain.  @chan can be NULL; @wait(NULL, @timeout) simply means sleep
  /// until @timeout
  ///
  /// @return 0 if wait() is signaled or ETIMEOUT if wait() times out
  virtual int wait(void *chan, unsigned timeout=FOREVER) { 
    incTurnCount(__PRETTY_FUNCTION__);
    putTurn();
    getTurn();
    return 0; 
  }

  /// wake up one thread (@all = false) or all threads (@all = true)
  /// waiting on @chan; must call with turn held.  @chan has the same
  /// requirement as wait()
  virtual void signal(void *chan, bool all = false) { }

  /// get the turn so that other threads trying to get the turn must wait
  virtual void getTurn() { }

  /// give up the turn so that other threads can get the turn.  this
  /// method should also increment turnCount
  virtual void putTurn(bool at_thread_end=false) { }

  /// inform the serializer that a thread is calling an external blocking
  /// function.
  ///
  /// NOTICE: different delay before @block() should not lead to different
  /// schedule.
  virtual int block(const char *callerName) { 
    getTurn();
    int ret = incTurnCount(__PRETTY_FUNCTION__); 
    putTurn();
    return ret;
  }

  /// start to fastly and safely do a inter-process operation (without getting a turn and putting it).
  /// by default it is NOP (always return false), if any runtime scheduler needs it, just reimplement it.
  virtual bool interProStart() {
    return false;
  }

  /// finish doing a inter-process operation.
  /// by default it is NOP (always return false), if any runtime scheduler needs it, just reimplement it.
  virtual bool interProEnd() {
    return false;
  }

  /// inform the scheduler that a blocking thread has returned.
  virtual void wakeup() {}

  /// inform the serializer that thread @new_th is just created; must call
  /// with turn held
  void create(pthread_t new_th) { TidMap::create(new_th); }

  /// inform the serializer that thread @th just joined; must call with
  /// turn held
  void join(pthread_t th) { TidMap::reap(th); }

  /// child process begins
  void childForkReturn() { TidMap::reset(pthread_self()); reopenLogs(); }
  void reopenLogs();

  /// NOTE: RecordSerializer needs this method to implement
  /// pthread_cond_*wait
  pthread_mutex_t *getLock() { return NULL; }

  /// must call within turn because turnCount is shared across all
  /// threads.  we provide this method instead of incrementing turn for
  /// each successful getTurn() because a successful getTurn() may not
  /// lead to a real success of a synchronization operation (e.g., see
  /// pthread_mutex_lock() implementation)
  static const int INF = 0x7fffff00;
  virtual unsigned incTurnCount(const char *callerName, unsigned delta = 0);
  virtual unsigned getTurnCount(void);

  void logNetworkOutput(int tid, const char *callerName, const void *buf, unsigned len);

  Serializer();
  ~Serializer();

  FILE *logger;
  FILE *loggerLight;
  FILE *loggerOutput;
  unsigned turnCount; // number of turns so far
};


///  @Scheduler defines the interface for a deterministic scheduler that
///  ensures that all synchronizations are deterministically scheduled.
///  The DMT runtime uses DMT schedulers.
///
///  The @wait() method method gives up the turn and then re-grabs it upon
///  return.  They are similar to pthread_cond_wait() and
///  pthread_cond_timedwait() which gives up the corresponding lock then
///  re-grabs it.
///
///  Note the Scheduler methods are not virtual by design.  The reason is
///  that pairing up a Scheduler subclass with a Runtime subclass requires
///  rewriting some of the Runtime methods.  That is, we cannot simply
///  plug in a different Scheduler subclass to an existing Runtime
///  subclass.  For this reason, we use the Scheduler subclasses as type
///  arguments to templated Runtime classes, and use method specialization
///  when necessary.  A nice side effect is we get static polymorphism
///  within the Runtime subclasses
struct Scheduler: public Serializer {

  /// wake up one thread (@all = false) or all threads (@all = true)
  /// waiting on @chan; must call with turn held.  @chan has the same
  /// requirement as wait()
  void signal(void *chan, bool all = false) { }

  void create(pthread_t new_th) {
    assert(self() == runq.front());
    TidMap::create(new_th);
    int tid = getTid(new_th);
    runq.create_thd_elem(tid);
    runq.push_back(tid);
  }

  void childForkReturn() {
    Serializer::childForkReturn();
    TidMap::reset(pthread_self());
    waitq.clear();
    runq.deep_clear();    
    struct run_queue::runq_elem *elem = runq.create_thd_elem(MainThreadTid);
    elem->status = run_queue::RUNNING_REG; // Pass the first token to the main thread after fork.
    runq.push_back(MainThreadTid);
    // Note: no need to clean up non_det_thds here, because they are only thread integer ids, not pointers in runq.
  }

  run_queue runq;
  std::list<int>  waitq;
  non_det_thread_set non_det_thds;
};


}

#endif
