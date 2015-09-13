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
//
// TODO:
// 1. use multiple wait queues per waitvar, instead of one for all
//    waitvars, for performance?
// 2. implement random scheduler
// 3. implement replay scheduler
// 4. support break out of turn.  RR can deadlock if program uses ad hoc
//    sync, such as "while(flag)"

#ifndef __TERN_RECORDER_SCHEDULER_H
#define __TERN_RECORDER_SCHEDULER_H

#include <list>
#include <vector>
#include <map>
#include <errno.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <semaphore.h>
#include <tr1/unordered_set>
#include "tern/runtime/scheduler.h"


namespace tern {

/// whoever comes first run; nondeterministic
struct RecordSerializer: public Serializer {
  typedef Serializer Parent;

  virtual void getTurn() { pthread_mutex_lock(&lock); }
  virtual void putTurn(bool at_thread_end = false) {
    if(at_thread_end)
      zombify(pthread_self());
    pthread_mutex_unlock(&lock);
  }

  int  wait(void *chan, unsigned timeout = Scheduler::FOREVER) {
    incTurnCount(__PRETTY_FUNCTION__);
    putTurn();
    sched_yield();  //  give control to other threads
    getTurn();
    return 0;
  }

  /// NOTE: This method breaks the Seralizer interface.  Need it to
  /// deterministically record pthread_cond_wait.  See the comments for
  /// pthread_cond_wait in the recorder runtime
  pthread_mutex_t *getLock() {
    return &lock;
  }

  ~RecordSerializer() {
    ouf.close();
  }

  std::ofstream ouf;

  RecordSerializer(): ouf("fsfs_message.log") {
    pthread_mutex_init(&lock, NULL);
  }

protected:
  pthread_mutex_t lock; // protects TidMap data
};


/// TODO: one optimization is to change the single wait queue to be
/// multiple wait queues keyed by the address they wait on, therefore no
/// need to scan the mixed wait queue.
struct RRScheduler: public Scheduler {
  typedef Scheduler Parent;
  
  struct wait_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    sem_t    sem;
    void*    chan;
    unsigned timeout;
    int      status; // return value of wait()
    volatile bool wakenUp;

    void reset(int st=0) {
      chan = NULL;
      timeout = FOREVER;
      status = st;
      wakenUp = false;
    }

    wait_t() {
      pthread_mutex_init(&mutex, NULL);
      pthread_cond_init(&cond, NULL);
      sem_init(&sem, 0, 0);
      reset(0);
    }    
    void wait();
    void post();
  }__attribute__((aligned(64)));  // Typical cache alignment.

  virtual void getTurn();
  virtual void putTurn(bool at_thread_end = false);
  virtual int  wait(void *chan, unsigned timeout = Scheduler::FOREVER);
  virtual void signal(void *chan, bool all=false);

  virtual int block(const char *callerName); 
  virtual bool interProStart();
  virtual bool interProEnd();
  virtual void wakeup();

  unsigned incTurnCount(const char *callerName, unsigned delta = 0);
  unsigned getTurnCount(void);

  void childForkReturn();

  RRScheduler();
  ~RRScheduler();

protected:

  ///Fast forward all limited timeout values.
  int forwardTimeouts(unsigned delta);
  /// timeout threads on @waitq
  int fireTimeouts();
  /// return the next timeout turn number
  unsigned nextTimeout();
  /// pop the @runq and wakes up the thread at the front of @runq
  virtual void next(bool at_thread_end=false, bool hasPoppedFront = false);
  /// child classes can override this method to reorder threads in @runq
  virtual void reorderRunq(void) {}

  /// for debugging
  void selfcheck(void);
  std::ostream& dump(std::ostream& o);
  /// An associated function to assist the fast and safe runq removal mechanism for network operation.
  /// Return the  ext runnable thread id. If this function returns an invalid tid, it means it is already the end of 
  /// execution of the program.
  int nextRunnable(bool at_thread_end = false);

  /// Called by the idle thread to decide whether the try put turn could be successful.
  /// If so, this function will modify the first runnable thread's status from RUNNABLE to RUNNING_REG,
  /// and the function return true. This is necessary because of the network handling mechanism,
  /// the runq "picture" seen by an idle thread could be racy (some threads may be runnable when the
  /// idle thread looks at the runq, but after the idle thread decides to cond wait, all threads in the runq
  /// can be calling blocking network operations). So we need this tryPutTurn().
  bool tryPutTurn();

  // MAYBE: can use a thread-local wait struct for each thread if it
  // improves performance
  wait_t waits[MAX_THREAD_NUM];

  //  for inter-process operation wakeup
  typedef std::tr1::unordered_set<int> tid_set;
  tid_set inter_pro_wakeup_tids;
  bool inter_pro_wakeup_flag;
  pthread_mutex_t inter_pro_wakeup_mutex;
  void check_wakeup();

  // For idle thread.
  void wakeUpIdleThread();
  void idleThreadCondWait();

  /** Check the current logical clock with threads in the non-det thread set,
  if current logical clock exceeds a bound with the thread with minimal logical clock in this set, block 
  the deterministic part of the system by adding this thread back to run queue.
  This function must be called by the run queue head thread while holding the global token.
  Precisely, this function is called within putTurn(). The reason I don't put it in next() or
  check_wakeup() is because the block() function will also call next(), and in block(), one thread
  may be just put into the non-det regions, and then immediately we check the non-det regions and
  try to put it back, which may be slow (only for performance consideration so far). **/
  void checkNonDetBound(); 
};

/// adapted from an example in POSIX.1-2001
struct Random {
  Random(): next(1) {}
  int rand(int randmax=32767)
  {
    next = next * 1103515245 + 12345;
    return (int)((unsigned)(next/65536) % (randmax + 1));
  }
  void srand(unsigned seed)
  {
    next = seed;
  }
  unsigned long next;
};

} // namespace tern

#endif
