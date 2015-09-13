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

#include "tern/runtime/record-scheduler.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#include <stdlib.h>
#include <cstring>
#include <algorithm>
#include <sched.h>
#include <sys/time.h>
#include "tern/options.h"
#include "tern/runtime/rdtsc.h"

using namespace std;
using namespace tern;

//#undef run_queue
//#define run_queue list<int>

//#define _DEBUG_RECORDER

#ifdef _DEBUG_RECORDER
#  define SELFCHECK  dump(cerr); selfcheck()
#  define dprintf(fmt...) do {                   \
     fprintf(stderr, "[%d] ", self());            \
     fprintf(stderr, fmt);                       \
     fflush(stderr);                             \
   } while(0)
#else
#  define SELFCHECK
#  define dprintf(fmt...) ;
#endif


extern pthread_mutex_t idle_mutex;
extern pthread_cond_t idle_cond;
extern int idle_done;

extern int nNonDetWait;
extern pthread_cond_t nonDetCV;


void RRScheduler::wait_t::wait() {
  if (options::enforce_turn_type == 1) {  // Semaphore relay.
    sem_wait(&sem);
  } else if (options::enforce_turn_type == 2) {  // Hybrid relay.
    /** by default, 3e4. This would cause the busy loop to loop for around 10 ms 
    on my machine, or 14 ms on bug00. This is one order of magnitude bigger
    than context switch time (1ms). **/
    /**
    2013-2-17: changed it to 4e5. It makes the parsec/flui* benchmark runs 
    with very small overhead (2~4 busywait timeouts) on bug00. If we choose 4e4, then there
    would be hundredsof timeouts.
    **/
    /** Heming: 2015-3-14: changed to be 4e8, because 
    in Paxos joint scheduling, some threads have to busy wait
    and then wait for the logical clocks from proxy, which could
    take about 1ms (paxos consensus). We enlarge this number to
    avoid context switch for DMT threads. **/
    const long waitCnt = 4e8;
    volatile long i = 0;
    while (!wakenUp && i < waitCnt) {
      sched_yield();
      i++;
    }
    if (!wakenUp) {
      pthread_mutex_lock(&mutex);
      while (!wakenUp) {/** This can save the context switch overhead. **/
        dprintf("RRScheduler::wait_t::wait before cond wait, tid %d\n", self());
        pthread_cond_wait(&cond, &mutex);
        dprintf("RRScheduler::wait_t::wait after cond wait, tid %d\n", self());
      }
      wakenUp = false;
      pthread_mutex_unlock(&mutex);
    } else {
      wakenUp = false;
    }
  } else {  // Busy relay.
    while (!wakenUp) {
      sched_yield();
    }
    wakenUp = false;
  }
}

void RRScheduler::wait_t::post() {
  if (options::enforce_turn_type == 1) { // Semaphore relay.
    sem_post(&sem);
  } else if (options::enforce_turn_type == 2) {   // Hybrid relay.
    pthread_mutex_lock(&mutex);
    wakenUp = true;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
  } else {  // Busy relay.
    //pthread_mutex_lock(&mutex);
    wakenUp = true;
    //pthread_mutex_unlock(&mutex);
  }
}

//@before with turn
//@after with turn
unsigned RRScheduler::nextTimeout()
{
  unsigned next_timeout = FOREVER;
  list<int>::iterator i;
  for(i=waitq.begin(); i!=waitq.end(); ++i) {
    int t = *i;
    if(waits[t].timeout < next_timeout)
      next_timeout = waits[t].timeout;
  }
  return next_timeout;
}

//@before with turn
//@after with turn
int RRScheduler::forwardTimeouts(unsigned delta)
{
  int num_forward = 0;
  list<int>::iterator prv, cur;
  // use delete-safe way of iterating the list
  for(cur=waitq.begin(); cur!=waitq.end();) {
    prv = cur ++;
    int tid = *prv;
    assert(tid >=0 && tid < Scheduler::nthread);
    if(waits[tid].timeout < FOREVER) {
      dprintf( "RRScheduler: %d forward timed out (%p, old %u --> new %u)\n",
              tid, waits[tid].chan, waits[tid].timeout, waits[tid].timeout + delta);
      waits[tid].timeout = waits[tid].timeout + delta;
      ++ num_forward;
    }
  }
  SELFCHECK;
  return num_forward;
}

//@before with turn
//@after with turn
int RRScheduler::fireTimeouts()
{
  int timedout = 0;
  list<int>::iterator prv, cur;
  // use delete-safe way of iterating the list
  for(cur=waitq.begin(); cur!=waitq.end();) {
    prv = cur ++;
    int tid = *prv;
    assert(tid >=0 && tid < Scheduler::nthread);
    if(waits[tid].timeout < turnCount) {
      dprintf("RRScheduler: %d timed out (%p, %u)\n",
              tid, waits[tid].chan, waits[tid].timeout);
      waits[tid].reset(ETIMEDOUT);
      waitq.erase(prv);
      runq.push_back(tid);
      ++ timedout;
    }
  }
  SELFCHECK;
  return timedout;
}

void RRScheduler::check_wakeup()
{
  if (inter_pro_wakeup_flag) {
    pthread_mutex_lock(&inter_pro_wakeup_mutex);
    for (tid_set::iterator itr = inter_pro_wakeup_tids.begin(); itr != inter_pro_wakeup_tids.end(); ++itr) {
      // This runq.in() call is safe, because check_wakeup() can only be called by 
      // the thread holding the turn.
      if (!runq.in(*itr)) {
        runq.push_back(*itr);
        if (options::enforce_non_det_clock_bound) {
          non_det_thds.erase(*itr); // This operation is required by the bounded non-determinism mechanism.
        }
      }
    }
    inter_pro_wakeup_tids.clear();
    inter_pro_wakeup_flag = false;
    pthread_mutex_unlock(&inter_pro_wakeup_mutex);
  }
}

//@before with turn
//@after with turn
void RRScheduler::next(bool at_thread_end, bool hasPoppedFront)
{
  int tid = self();
  int next_tid;
  if (!hasPoppedFront) {
    // Update the status of the head element.
    struct run_queue::runq_elem *my = runq.get_my_elem(tid);
    dprintf("RRScheduler::nextRunnable at_thread_end %d, self tid %d, head status %d\n",
      at_thread_end, tid, my->status);
    if (my->status == run_queue::RUNNING_REG)
      my->status = run_queue::RUNNABLE;
    else {
      assert(my->status == run_queue::RUNNING_INTER_PRO);
      my->status = run_queue::INTER_PRO_STOP;
    }

    // remove self from runq
    runq.pop_front();
  }
  
  check_wakeup();

  next_tid = nextRunnable(at_thread_end);
  // There are two special cases that: (1) at the thread end, waitq is empty, or 
  // (2) main thread exits (and waitq can be non-empty, e.g., openmp),
  // then we do not need to pass turn any more and just return.
  if (next_tid == InvalidTid)
    return;

  // reorderRunq(); Heming: do not call this function, even it is implemented in seeded 
  // RR. This reordering is conflicting with RR scheduling (with network).

  assert(next_tid>=0 && next_tid < Scheduler::nthread);
  dprintf("RRScheduler: next is %d\n", next_tid);
  SELFCHECK;

  waits[next_tid].post();
}

void RRScheduler::wakeUpIdleThread() {
  int tid = -1;
  if (idle_done) {
    fprintf(stderr, "WARN: idle thread is done, but tid %d is still running (for example, in OpenMP). Exit too.\n", self());
    fflush(stderr);
    pthread_exit(0);
  }
  list<int>::iterator prv, cur;
  // use delete-safe way of iterating the list
  for(cur=waitq.begin(); cur!=waitq.end();) {
    prv = cur ++;

    tid = *prv;
    assert(tid >=0 && tid < Scheduler::nthread);
    if(tid == IdleThreadTid) {
      waits[tid].reset();
      waitq.erase(prv);
      runq.push_back(tid);
      break;
    }
  }
  assert(tid == IdleThreadTid);
  assert(!runq.empty());
  pthread_mutex_lock(&idle_mutex);
  pthread_cond_signal(&idle_cond);
  pthread_mutex_unlock(&idle_mutex);
}

void RRScheduler::idleThreadCondWait() {
  /** At this moment the idle thread is still holding the turn, but the 
  "picture" of run queue can be racy due to the fast and safe networking 
  removal mechanism. Some threads may be runnable when the
  idle thread looks at the runq, but after the idle thread decides to cond wait,
  all threads in the runq can be calling blocking network operations.
  So we need tryPutTurn(). **/
  if (tryPutTurn()) {
    int tid = self();
    assert(tid == IdleThreadTid);
    waits[tid].chan = (void *)&idle_cond;
    waits[tid].timeout = FOREVER;
    waitq.push_back(tid);
    assert(tid == runq.front());
    next();
    pthread_cond_wait(&idle_cond, &idle_mutex);
  } else
    putTurn();  // TBD: this seems not that nice, need refactored. Refer to record-runtime.
}

//@before without turn
//@after with turn
void RRScheduler::getTurn()
{
  int tid = self();
  assert(tid>=0 && tid < Scheduler::nthread);
  waits[tid].wait();
  dprintf("RRScheduler: %d gets turn\n", self());

  SELFCHECK;
}

int RRScheduler::block(const char *callerName)
{
  getTurn();
  int tid = self();
  if (options::enforce_non_det_clock_bound)
    non_det_thds.insert(tid, turnCount); // This operation is required by the bounded non-determinism mechanism.
  assert(tid>=0 && tid < Scheduler::nthread);
  assert(tid == runq.front());
  dprintf("RRScheduler: %d blocks\n", self());
  std::string funcName = callerName;
  funcName += "  ||||  ";
  funcName += __PRETTY_FUNCTION__;
  int ret = incTurnCount(funcName.c_str());
  next();
  return ret;
}

void RRScheduler::wakeup()
{
  pthread_mutex_lock(&inter_pro_wakeup_mutex);
  inter_pro_wakeup_tids.insert(self());
  inter_pro_wakeup_flag = true;
  pthread_mutex_unlock(&inter_pro_wakeup_mutex);
}

//@before with turn
//@after without turn
void RRScheduler::putTurn(bool at_thread_end)
{
  int tid = self();
  //if (tid == 2)
    //runq.dbg_print(__FUNCTION__);
  assert(tid>=0 && tid < Scheduler::nthread);
  assert(tid == runq.front());
  bool hasPoppedFront = false;

  if(at_thread_end) {
    signal((void*)pthread_self());
    Parent::zombify(pthread_self());
    dprintf("RRScheduler: %d ends\n", self());
  } else {
    // Check and modify "my" run queue element. No need to grab element spinlock since I am the head.
    struct run_queue::runq_elem *my = runq.get_my_elem(tid);
    // Current if branch can not be taken (hasPoppedFront is false) if a thread
    // is doing network operation, so current status must be RUNNING_REG.
    assert (my->status == run_queue::RUNNING_REG);
    my->status = run_queue::RUNNABLE;

    // Process run queue structure.
    runq.pop_front();
    hasPoppedFront = true;
    runq.push_back(tid);
    dprintf("RRScheduler: %d puts turn\n", self());
  }

  // Enforce bounded non-determinism.
  checkNonDetBound();

  next(at_thread_end, hasPoppedFront);
}

//@before with turn
//@after with turn
int RRScheduler::wait(void *chan, unsigned nturn)
{
  record_rdtsc_op("RRScheduler::wait", "START", 2, NULL); // record rdtsc, disabled by default, no performance impact.
  incTurnCount(__PRETTY_FUNCTION__);
  int tid = self();
  assert(tid>=0 && tid < Scheduler::nthread);
  assert(tid == runq.front());
  waits[tid].chan = chan;
  waits[tid].timeout = nturn;
  waitq.push_back(tid);
  dprintf("RRScheduler: %d waits on (%p, %u)\n", tid, chan, nturn);

  next();

  getTurn();
  record_rdtsc_op("RRScheduler::wait", "END", 2, NULL); // record rdtsc, disabled by default, no performance impact.
  return waits[tid].status;
}

//@before with turn
//@after with turn
void RRScheduler::signal(void *chan, bool all)
{
  list<int>::iterator prv, cur;
  assert(chan && "can't signal/broadcast NULL");
  assert(self() == runq.front());
  dprintf("RRScheduler: %d: %s %p\n",
          self(), (all?"broadcast":"signal"), chan);

  // use delete-safe way of iterating the list in case @all is true
  for(cur=waitq.begin(); cur!=waitq.end();) {
    prv = cur ++;

    int tid = *prv;
    assert(tid >=0 && tid < Scheduler::nthread);
    if(waits[tid].chan == chan) {
      dprintf("RRScheduler: %d signals tid %d(%p)\n", self(), tid, chan);
      waits[tid].reset();
      waitq.erase(prv);
      runq.push_back(tid);
      if(!all)
        break;
    }
  }
  SELFCHECK;
}

//@before with turn
//@after with turn
unsigned RRScheduler::incTurnCount(const char *callerName, unsigned delta)
{
  unsigned ret = Serializer::incTurnCount(callerName, delta);
  if (delta > 0)
    forwardTimeouts(delta);
  fireTimeouts();
  check_wakeup();
  return ret;
}

unsigned RRScheduler::getTurnCount(void)
{
  return Serializer::getTurnCount();
}

void RRScheduler::childForkReturn() {
  Parent::childForkReturn();
  for(int i=0; i<MAX_THREAD_NUM; ++i)
    waits[i].reset();

  inter_pro_wakeup_tids.clear();
  inter_pro_wakeup_flag = 0;
  pthread_mutex_init(&inter_pro_wakeup_mutex, NULL);
}


RRScheduler::~RRScheduler() {}

RRScheduler::RRScheduler()
{
  // main thread
  assert(self() == MainThreadTid && "tid hasn't been initialized!");
  struct run_queue::runq_elem *main_elem = runq.create_thd_elem(MainThreadTid);
  runq.push_back(self());
  waits[MainThreadTid].post(); // Assign an initial turn to main thread.
  main_elem->status = run_queue::RUNNING_REG;// Assign an initial running state (i.e., turn) to main thread.

  inter_pro_wakeup_tids.clear();
  inter_pro_wakeup_flag = 0;
  pthread_mutex_init(&inter_pro_wakeup_mutex, NULL);
}

void RRScheduler::selfcheck(void)
{
  fprintf(stderr, "RRScheduler::selfcheck tid %d\n", self());
  tr1::unordered_set<int> tids;

  // no duplicate tids on runq
  for(run_queue::iterator th=runq.begin(); th!=runq.end(); ++th) {
    if(*th < 0 || *th > Scheduler::nthread) {
      dump(cerr);
      assert(0 && "invalid tid on runq!");
    }
    if(tids.find(*th) != tids.end()) {
      dump(cerr);
      assert(0 && "duplicate tids on runq!");
    }
    tids.insert(*th);
  }

  // no duplicate tids on waitq
  for(list<int>::iterator th=waitq.begin(); th!=waitq.end(); ++th) {
    if(*th < 0 || *th > Scheduler::nthread) {
      dump(cerr);
      assert(0 && "invalid tid on waitq!");
    }
    if(tids.find(*th) != tids.end()) {
      dump(cerr);
      assert(0 && "duplicate tids on runq or waitq!");
    }
    tids.insert(*th);
  }

  // TODO: check that tids have all tids

  // threads on runq have NULL chan or non-forever timeout
  for(run_queue::iterator th=runq.begin(); th!=runq.end(); ++th)
    if(waits[*th].chan != NULL || waits[*th].timeout != FOREVER) {
      dump(cerr);
      assert(0 && "thread on runq but has non-NULL chan "\
             "or non-zero turns left!");
    }

  // threads on waitq have non-NULL waitvars or non-zero timeout
  for(list<int>::iterator th=waitq.begin(); th!=waitq.end(); ++th)
    if(waits[*th].chan == NULL && waits[*th].timeout == FOREVER) {
      dump(cerr);
      assert (0 && "thread on waitq but has NULL chan and 0 turn left!");
    }
}

ostream& RRScheduler::dump(ostream& o)
{
  o << "nthread " << Scheduler::nthread << ": " << turnCount;
  o << " [runq ";
  copy(runq.begin(), runq.end(), ostream_iterator<int>(o, " "));
  o << "]";
  o << " [waitq ";
  for(list<int>::iterator th=waitq.begin(); th!=waitq.end(); ++th)
    o << *th << "(" << waits[*th].chan << "," << waits[*th].timeout << ") ";
  o << "]\n";
  return o;
}

bool RRScheduler::interProStart() {
  bool isHead = true;
  struct run_queue::runq_elem *elem = runq.get_my_elem(self());

  pthread_spin_lock(&elem->spin_lock);
  if (elem->status == run_queue::RUNNABLE) {
    isHead = false;
    elem->status = run_queue::INTER_PRO_STOP;
  } else {
    assert(elem->status == run_queue::RUNNING_REG);
    elem->status = run_queue::RUNNING_INTER_PRO;
  }
  pthread_spin_unlock(&elem->spin_lock);

  return isHead;
}

bool RRScheduler::interProEnd() {
  struct run_queue::runq_elem *elem = runq.get_my_elem(self());
  pthread_spin_lock(&elem->spin_lock);
  assert(elem->status == run_queue::INTER_PRO_STOP);
  elem->status = run_queue::RUNNABLE;
  pthread_spin_unlock(&elem->spin_lock);
  return true;
}

int RRScheduler::nextRunnable(bool at_thread_end) {
  bool passed = false;
  
  struct run_queue::runq_elem *headElem = NULL;
  while (true) { // This loop is guaranteed to finish.
    // If run queue is empty, wake up idle thread.
    if(runq.empty()) {
      // Current thread must be the last thread and it is existing, otherwise we wake up the idle thread.
      // There are two special cases that: (1) at the thread end, waitq is empty, or 
      // (2) main thread exits (and waitq can be non-empty, e.g., openmp),
      // then just return an invalid tid.
      if (at_thread_end && waitq.empty()) {
        return InvalidTid;
      } else if (at_thread_end && !waitq.empty() && self() == MainThreadTid) {
        fprintf(stderr, "WARNING: main thread exits with some children threads alive (e.g., openmp).\n");
        return InvalidTid;
      } else {
        if (!options::launch_idle_thread) {
          fprintf(stderr, "WARN: the program may contain some blocking network \
            operations which requires 'options::launch_idle_thread = 1', please \
            check your local.options file and rerun.\n");
          exit(1);
        }
        assert(self() != IdleThreadTid);
        wakeUpIdleThread();
      }
    } else {
      /* If runq only contains idle thread and there are threads blocking on 
      non-det-start, then just wake them up. */
      if (options::enforce_non_det_annotations && nNonDetWait > 0 &&
        self() == IdleThreadTid && runq.size() == 1 && runq.front() == IdleThreadTid) {
        dprintf("nextRunnable() Tid %d wakes up nonDet start threads\n", self());
        signal(&nonDetCV, true);
      }
    }
    assert(!runq.empty());

    // Process one head element.
    headElem = runq.front_elem();
    pthread_spin_lock(&headElem->spin_lock);
    if (headElem->status == run_queue::INTER_PRO_STOP) {
      /** If this thread is blocking, remove it from run queue
      and find the next one. The head thread is the only thread
      that could modify the linked list of run queue, so it is safe. **/
      runq.pop_front();  
      if (options::enforce_non_det_clock_bound)
        non_det_thds.insert(headElem->tid, turnCount); // This operation is required by the bounded non-determinism mechanism.
    } else {
      dprintf("RRScheduler::nextRunnable at_thread_end %d, self %d, headElem tid %d, head status %d, self status %d\n",
        at_thread_end, self(), headElem->tid, headElem->status, runq.get_my_elem(self())->status);
      assert(headElem->status == run_queue::RUNNABLE ||
        headElem->status == run_queue::RUNNING_REG || 
        headElem->status == run_queue::RUNNING_INTER_PRO);
      if (headElem->status == run_queue::RUNNABLE)
        headElem->status = run_queue::RUNNING_REG;
      passed = true;
    }
    pthread_spin_unlock(&headElem->spin_lock);
 
    if (passed)
      break;
  }

  assert(headElem);
  return headElem->tid;
}

bool RRScheduler::tryPutTurn() {
  assert(!runq.empty());
  assert(self() == runq.front());
  run_queue::iterator itr = runq.begin();
  itr++; // Ignore myself.
  for (; itr != runq.end(); ++itr) {
    struct run_queue::runq_elem *cur = &itr;
    pthread_spin_lock(&cur->spin_lock);
    if (cur->status == run_queue::RUNNABLE) {
      cur->status = run_queue::RUNNING_REG; // Try put turn succeeded.
      pthread_spin_unlock(&cur->spin_lock);
      return true;
    }
    pthread_spin_unlock(&cur->spin_lock);
  }
  return false;
}

void RRScheduler::checkNonDetBound() { 
  if (options::enforce_non_det_clock_bound && non_det_thds.size() > 0) {
    int tid = non_det_thds.first_thread();
    unsigned clock = non_det_thds.get_clock(tid);
    if (turnCount > clock + options::non_det_clock_bound) {
      //assert(!runq.in(tid));
      runq.push_back(tid);
      non_det_thds.erase(tid);
      dprintf("checkNonDetBound: current logical clock %u, first non det tid %d, my tid %d, non det logical clock %u, \
        try to block the deterministict part of the system.\n", turnCount, tid, self(), clock);
    }
  }
}

