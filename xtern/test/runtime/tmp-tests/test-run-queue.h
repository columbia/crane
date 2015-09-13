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

#include <iterator>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#define MAX_THREAD_NUM 1000

class run_queue {
  struct runq_elem {
    pthread_spinlock_t spin_lock;
    int tid;
    int status;
    struct runq_elem *prev;
    struct runq_elem *next;
  };
  
  struct runq_elem *head;
  struct runq_elem *tail;
  size_t num_elements;
  struct runq_elem *tid_map[MAX_THREAD_NUM];

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
    clear();
    for (unsigned i = 0; i < MAX_THREAD_NUM; i++)
      tid_map[i] = NULL;
  }

  struct runq_elem *create_thd_elem(int tid) {
    struct runq_elem *elem = new struct runq_elem;
    pthread_spin_init(&(elem->spin_lock), 0);
    elem->tid = tid;
    elem->status = 0; // XXX TO BD DONE.
    elem->prev = NULL;
    elem->next = NULL;
    tid_map[tid] = elem;
    return elem;
  }

  void del_thd_elem(int tid) {
    struct runq_elem *elem = tid_map[tid];
    assert(elem);
    tid_map[tid] = NULL;
    pthread_spin_destroy(&(elem->spin_lock));
    delete elem;
  }

  inline iterator begin() {
    return iterator(head);
  }

  inline iterator end() {
    return iterator();
  }

  void clear() {
    head = tail = NULL;
    num_elements = 0;
  }

  bool empty() {
    return (size() == 0);
  }
 
  size_t size() {
    return num_elements;
  }

  // Complicated, need more check.
  iterator erase (iterator position) {
    if (position == end()) {
      fprintf(stderr, "erase0\n");
      return end();
    } else {
      fprintf(stderr, "erase1\n");
      struct runq_elem *ret = position->next;

      fprintf(stderr, "erase2\n");
      // Connect the "new" prev and next.
      if (position->prev != NULL)
        position->prev->next = position->next;
      if (position->next != NULL)
        position->next->prev = position->prev;

      fprintf(stderr, "erase3\n");
      // Process head and tail.
      if (iterator(head) == position)
        head = position->next;
      if (iterator(tail) == position)
        tail = position->prev;

      fprintf(stderr, "erase4\n");
      num_elements--;
      return iterator(ret);
    }
  }
  
  void push_back(int tid) {
    struct runq_elem *elem = tid_map[tid];
    assert(elem);
    if (head == NULL) {
      assert(tail == NULL);
      head = tail = elem;
    } else {
      assert (tail != NULL);
      elem->prev = tail;
      tail->next = elem;
      tail = elem;
    }
    num_elements++;
  }

  /* This is a thread run queue, all thread ids are fixed, reference is not allowed! */
  int front() {
    assert(head != NULL);
    return head->tid;
  }

  void push_front(int tid) {
    struct runq_elem *elem = tid_map[tid];
    assert(elem);
    if (head == NULL) {
      head = tail = elem;
    } else {
      elem->next = head;
      head = elem;
    }
    num_elements++;
  }

  void pop_front() {
    struct runq_elem *elem = head;
    head = elem->next;
    if (head == NULL) /** If head is empty, then the tail must also be empty. **/
      tail = NULL;
    num_elements--;
  }
};

