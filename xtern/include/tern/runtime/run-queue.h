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

#ifndef __TERN_COMMON_RUNTIME_QUEUE_H
#define __TERN_COMMON_RUNTIME_QUEUE_H

#include <iterator>
#include <tr1/unordered_set>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_THREAD_NUM 5000///1111
//#define DEBUG_RUN_QUEUE // "defined" means enable the debug check; "undef" means disable it (faster).

#ifdef DEBUG_RUN_QUEUE
#define DBG_ASSERT_ELEM_IN(...) dbg_assert_elem_in(__VA_ARGS__)
#else
#define DBG_ASSERT_ELEM_IN(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define DBG_ASSERT_ELEM_NOT_IN(...) dbg_assert_elem_not_in(__VA_ARGS__)
#else
#define DBG_ASSERT_ELEM_NOT_IN(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define DBG_INSERT_ELEM(...) dbg_insert_elem(__VA_ARGS__)
#else
#define DBG_INSERT_ELEM(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define DBG_ERASE_ELEM(...) dbg_erase_elem(__VA_ARGS__)
#else
#define DBG_ERASE_ELEM(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define DBG_CLEAR_ALL_ELEMS(...) dbg_clear_all_elems(__VA_ARGS__)
#else
#define DBG_CLEAR_ALL_ELEMS(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define DBG_ASSERT_ELEM_SIZE(...) dbg_assert_elem_size(__VA_ARGS__)
#else
#define DBG_ASSERT_ELEM_SIZE(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define PRINT(...) print(__VA_ARGS__)
#else
#define PRINT(...)
#endif

#ifdef DEBUG_RUN_QUEUE
#define ASSERT(...) assert(__VA_ARGS__)
#else
#define ASSERT(...)
#endif


namespace tern {
class run_queue {
public:
  enum THD_STATUS {
    RUNNABLE,     /** The thread can do any regular pthreads sync operation. **/
    RUNNING_REG,     /** The thread has got a turn and it may call any sync operation (can be pthreads or inter-process operation). **/
    RUNNING_INTER_PRO,      /** The thread has got a turn and it is going to call a inter-process operation. **/
    INTER_PRO_STOP      /** The thread is stopped (or blocked) on a inter-process operation, and no other thread can pass turn to it. **/
  };
  
  struct runq_elem {
  public:
    pthread_spinlock_t spin_lock;
    int tid;
    THD_STATUS status;
    struct runq_elem *prev;
    struct runq_elem *next;

    runq_elem(int tid) {
      pthread_spin_init(&spin_lock, 0);
      this->tid = tid;
      status = RUNNABLE;
      prev = next = NULL;
    }
  };

private:
  /** Key members of the run queue. We mainly optimize it for read/write of head/tail. **/
  struct runq_elem *head;
  struct runq_elem *tail;
  size_t num_elements;
  struct runq_elem *tid_map[MAX_THREAD_NUM];

  /** This one is useful only when DEBUG_RUN_QUEUE is defined. **/
  std::tr1::unordered_set<void *> elements;

public:
  class iterator : public std::iterator<std::forward_iterator_tag, int> {
    struct runq_elem *m_rep;
  public:
    friend class run_queue;

    inline iterator(struct runq_elem *x=0):m_rep(x){}
      
    inline iterator(const iterator &x):m_rep(x.m_rep) {}
      
    inline iterator& operator=(const iterator& x) { 
      m_rep=x.m_rep;
      return *this; 
    }

    inline iterator& operator++() { 
      m_rep = m_rep->next;
      return *this; 
    }

    inline iterator operator++(int) { 
      iterator tmp(*this);
      m_rep = m_rep->next;
      return tmp; 
    }

    inline reference operator*() const {
      return m_rep->tid;
    }

    inline struct runq_elem *operator&() const {
      return m_rep;
    }

    // This has compilation problem, so switch to the version below.
    // inline pionter operator->() { 
    inline struct runq_elem *operator->() {
      return m_rep;
    }

    inline bool operator==(const iterator& x) const {
      return m_rep == x.m_rep; 
    }	

    inline bool operator!=(const iterator& x) const {
      return m_rep != x.m_rep; 
    }
  };

  run_queue() {
    memset(tid_map, 0, sizeof(struct runq_elem *)*MAX_THREAD_NUM);
    deep_clear();
  }

  /** Each thread get its own thread element. This is a per-thread array so it is thread-safe. **/
  inline struct runq_elem *get_my_elem(int my_tid) {
#ifdef DEBUG_RUN_QUEUE
    struct runq_elem *elem = tid_map[my_tid];
    ASSERT(elem && my_tid == elem->tid); /** Make sure each thread can only get its own element. **/
    return elem;
#else
    return tid_map[my_tid];
#endif
  }
  
  inline struct runq_elem *create_thd_elem(int tid) {
    //fprintf(stderr, "tid %d is called with runq::create_thd_elem\n", tid);
    ASSERT(tid >= 0 && tid < MAX_THREAD_NUM);
    ASSERT(tid_map[tid] == NULL);
    struct runq_elem *elem = new runq_elem(tid);
    tid_map[tid] = elem;
    return elem;
  }

  inline void del_thd_elem(int tid) {
    PRINT(__FUNCTION__);
    struct runq_elem *elem = tid_map[tid];
    ASSERT(elem);
    tid_map[tid] = NULL;
    pthread_spin_destroy(&(elem->spin_lock));
    delete elem;
  }

  inline void dbg_assert_elem_in(const char *tag, struct runq_elem *elem) {
#ifdef DEBUG_RUN_QUEUE
  if (elements.find((void *)elem) == elements.end()) {
    //fprintf(stderr, "DBG_FUNC: %s, elem tid %d, tag %s.\n", __FUNCTION__, elem?elem->tid:-1, tag);
    int i = 0;
    //fprintf(stderr, "\n\n OP: %s: elements set size %u\n", tag, (unsigned)elements.size());
    for (run_queue::iterator itr = begin(); itr != end(); ++itr) {
      if (i > MAX_THREAD_NUM)
        assert(false);
      //fprintf(stderr, "q[%d] = tid %d, status = %d\n", i, *itr, itr->status);
      i++;
    }
    assert(false);
  }
#endif
  }

  inline void dbg_assert_elem_not_in(const char *tag, struct runq_elem *elem) {
#ifdef DEBUG_RUN_QUEUE
    if (elements.find((void *)elem) != elements.end()) {
      fprintf(stderr, "DBG_FUNC: %s, elem tid %d, tag %s.\n", __FUNCTION__, elem?elem->tid:-1, tag);
      assert(false);
    }
#endif
  }

  inline void dbg_insert_elem(const char *tag, struct runq_elem *elem) {
#ifdef DEBUG_RUN_QUEUE
    elements.insert((void *)elem);
#endif
  }

  inline void dbg_erase_elem(const char *tag, struct runq_elem *elem) {
#ifdef DEBUG_RUN_QUEUE
    elements.erase((void *)elem);
#endif
  }

  inline void dbg_clear_all_elems() {
#ifdef DEBUG_RUN_QUEUE
    elements.clear();
#endif
  }

  void dbg_assert_elem_size(const char *tag, size_t sz) {
#ifdef DEBUG_RUN_QUEUE
      if (elements.size() != sz) {
        fprintf(stderr, "elements set size %u, num_elements %u\n", (unsigned)elements.size(), (unsigned)sz);
        assert(false);
      }
#endif
  }
  
  inline iterator begin() {
    return iterator(head);
  }

  inline iterator end() {
    return iterator();
  }

  /** Check whether current element is in the queue. Only the head-of run queue should call this function,
  because it is the only thread which could modify the linked list of run queue. **/
  inline bool in(int tid) {
    struct runq_elem *elem = tid_map[tid];
    ASSERT(elem);
    /** If I have prev or next element, then I am still in the queue. **/
    if (elem->prev != NULL || elem->next != NULL) {
      DBG_ASSERT_ELEM_IN("run_queue.in.1", elem);
      return true;
    }
    /** Else, if I am the only element in the queue, then I am still in the queue. **/
    else if (head == elem && tail == elem) {
      DBG_ASSERT_ELEM_IN("run_queue.in.2", elem);
      return true;
    }
    DBG_ASSERT_ELEM_NOT_IN("run_queue.in.3", elem);
    return false;
  }

  /** This is a "deep" clear. It not only clears the list, but also the tid_map.
  This function should only be called when handling fork() and a deep clean is requried. **/
  inline void deep_clear() {
    //PRINT(__FUNCTION__);
    head = tail = NULL;
    num_elements = 0;
    DBG_CLEAR_ALL_ELEMS();
    for (int i = 0; i < MAX_THREAD_NUM; i++) {// An un-opt version, TBD.
      if (tid_map[i] != NULL) {
        int tid = tid_map[i]->tid;
        tid_map[i]->prev = tid_map[i]->next = NULL;
        del_thd_elem(tid); // Deep clear.
      }
    }
  }

  inline bool empty() {
    PRINT(__FUNCTION__);
    return (size() == 0);
  }
 
  inline size_t size() {
    //PRINT(__FUNCTION__);
    DBG_ASSERT_ELEM_SIZE(__FUNCTION__, num_elements);
    return num_elements;
  }

  // Complicated, need more check.
  inline iterator erase (iterator position) {
    PRINT(__FUNCTION__);
    if (position == end()) {
      return end();
    } else {
      struct runq_elem *ret = position->next;
      struct runq_elem *cur = &position;
      DBG_ASSERT_ELEM_IN(__FUNCTION__, cur);

      // Connect the "new" prev and next.
      if (position->prev != NULL)
        position->prev->next = position->next;
      if (position->next != NULL)
        position->next->prev = position->prev;

      // Process head and tail.
      if (iterator(head) == position)
        head = position->next;
      if (iterator(tail) == position)
        tail = position->prev;

      // Clear the position's prev and next.
      cur->prev = cur->next = NULL;

      DBG_ERASE_ELEM(__FUNCTION__, cur);
      num_elements--;
      return iterator(ret);
    }
  }
  
  inline void push_back(int tid) {
    PRINT("push_back_start");
    //fprintf(stderr, "~~~~~~~~~~~~push back tid %d\n", tid);

    struct runq_elem *elem = tid_map[tid];
    ASSERT(elem);
    DBG_ASSERT_ELEM_NOT_IN(__FUNCTION__, elem);
    if (head == NULL) {
      ASSERT(tail == NULL);
      head = tail = elem;
    } else {
      ASSERT(tail != NULL);
      elem->prev = tail;
      tail->next = elem;
      tail = elem;
    }
    DBG_INSERT_ELEM(__FUNCTION__, elem);
    num_elements++;
    PRINT("push_back_end");
  }

  /* This is a thread run queue, all thread ids are fixed, reference is not allowed! */
  inline int front() {
    PRINT(__FUNCTION__);
    ASSERT(head != NULL);
    DBG_ASSERT_ELEM_IN(__FUNCTION__, head);
    return head->tid;
  }

  inline struct runq_elem *front_elem() {
    PRINT(__FUNCTION__);
    ASSERT(head != NULL);
    DBG_ASSERT_ELEM_IN(__FUNCTION__, head);
    return head;
  }

  inline void push_front(int tid) {
    PRINT(__FUNCTION__);
    struct runq_elem *elem = tid_map[tid];
    ASSERT(elem);
    DBG_ASSERT_ELEM_NOT_IN(__FUNCTION__, elem);
    if (head == NULL) {
      head = tail = elem;
    } else {
      elem->next = head;
      head = elem;
    }
    DBG_INSERT_ELEM(__FUNCTION__, elem);
    num_elements++;
  }

  inline void pop_front() {
    PRINT(__FUNCTION__);
    struct runq_elem *elem = head;
    DBG_ASSERT_ELEM_IN(__FUNCTION__, elem);
    head = elem->next;
    elem->prev = elem->next = NULL;
    if (head == NULL) /** If head is empty, then the tail must also be empty. **/
      tail = NULL;
    DBG_ERASE_ELEM(__FUNCTION__, elem);
    num_elements--;
  }

  inline void print(const char *tag) {
    return;
    int i = 0;
    fprintf(stderr, "\n\n OP: %s: elements set size %u\n", tag, (unsigned)elements.size());
    for (run_queue::iterator itr = begin(); itr != end(); ++itr) {
      if (i > MAX_THREAD_NUM)
        assert(false);
      fprintf(stderr, "q[%d] = tid %d, status = %d\n", i, *itr, itr->status);
      i++;
    }
  }

  inline void dbg_print(const char *tag) {
    //fprintf(stderr, "\n\n OP: %s: elements set size %u\n", tag, (unsigned)elements.size());
#ifdef DEBUG_RUN_QUEUE
    int i = 0;
    fprintf(stderr, "\n\n OP: %s: elements set size %u\n", tag, (unsigned)elements.size());
    for (run_queue::iterator itr = begin(); itr != end(); ++itr) {
      if (i > MAX_THREAD_NUM)
        assert(false);
      fprintf(stderr, "q[%d] = tid %d, status = %d\n", i, *itr, itr->status);
      i++;
    }
#endif
  }
};
}
#endif

