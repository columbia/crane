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

#ifndef __TERN_NON_DET_THREAD_SET_H
#define __TERN_NON_DET_THREAD_SET_H

//#include <iterator>
#include <tr1/unordered_set>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <list>
//#include <string.h>

#define DEBUG_NON_DET_THREAD_SET
#ifdef DEBUG_NON_DET_THREAD_SET
#define ASSERT2(...) assert(__VA_ARGS__)
#else
#define ASSERT2(...)
#endif

namespace tern {
struct non_det_thread_set {
  protected:
    std::list<int> tid_list;
    std::tr1::unordered_map<int, unsigned> tid_to_logical_clock_map;

  public:
    non_det_thread_set() {
      tid_list.clear();
      tid_to_logical_clock_map.clear();
    }
    
    void insert(int tid, unsigned clock) {
      //fprintf(stderr, "non-det-thread-set insert tid %d, clock %u\n", tid, clock);
      ASSERT2(!in(tid));
      tid_list.push_back(tid);
      ASSERT2(tid_to_logical_clock_map.find(tid) == tid_to_logical_clock_map.end());
      tid_to_logical_clock_map[tid] = clock;
    }

    void erase(int tid) {
      bool found = false;
      std::list<int>::iterator itr = tid_list.begin();
      for (; itr != tid_list.end(); itr++) { // TBD: this operation is linear, may be slow.
        if (tid == *itr) {
          found = true;
          break;
        }
      }
      if (found) {
        tid_list.erase(itr);
        tid_to_logical_clock_map.erase(tid);
      } else
        assert(false && "tid must be in the non det set."); // this assertion must be there.
    }

    size_t size() {
      ASSERT2(tid_list.size() == tid_to_logical_clock_map.size());
      return tid_list.size();
    }

    int first_thread() {
      ASSERT2(size()>0);
      return *(tid_list.begin());
    }

    unsigned get_clock(int tid) {
      ASSERT2(tid_to_logical_clock_map.find(tid) != tid_to_logical_clock_map.end());
      return tid_to_logical_clock_map[tid];
    }

    bool in(int tid) {
      if (tid_to_logical_clock_map.find(tid) != tid_to_logical_clock_map.end())
        return true;
      else {
        bool found = false;
        std::list<int>::iterator itr = tid_list.begin();
        for (; itr != tid_list.end(); itr++) {
          if (tid == *itr) {
            found = true;
            break;
          }
        }
        return found;
      }
    }
};
};
#endif
