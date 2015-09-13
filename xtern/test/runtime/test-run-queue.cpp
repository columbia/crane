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

// RUN: %srcroot/test/runtime/run-scheduler-test.py %s -gxx "%gxx" -objroot "%objroot" -ternruntime "%ternruntime" -ternannotlib "%ternannotlib"

/* A simple test file for the fast thread run queue. */

#include "../../include/tern/runtime/run-queue.h"

using namespace std;
using namespace tern;
run_queue q;

void print() {
  int i = 0;
  printf("q size %u\n", (unsigned)q.size());
  for (run_queue::iterator itr = q.begin(); itr != q.end(); itr++) {
    printf("q[%d] = %d, status = %d\n", i, *itr, itr->status);
    i++;
  }
}

int main(int argc, char *argv[]) {
  q.create_thd_elem(2);
  q.create_thd_elem(3);
  q.create_thd_elem(4);
  q.create_thd_elem(5);
  q.create_thd_elem(6);


  q.push_back(2);
  q.push_back(3);
  q.push_back(4);
  q.push_back(5);
  q.push_back(6);

  print();

 run_queue::iterator itr1 = q.begin();
 q.erase(itr1);
 print();


 run_queue::iterator itr2 = q.begin();
 ++itr2;
 q.erase(itr2);
 print();

 run_queue::iterator itr3 = q.end();
 q.erase(itr3);
 print();

 run_queue::iterator itr4 = q.begin();
 itr4++;
 itr4++;
 q.erase(itr4);
 print();


 int first = q.front();
 first = 99;
 print();

 q.push_front(4);
 print(); 

 q.pop_front();
 print(); 
}


// CHECK: q size 5
// CHECK-NEXT-L: q[0] = 2, status = 0
// CHECK-NEXT-L: q[1] = 3, status = 0
// CHECK-NEXT-L: q[2] = 4, status = 0
// CHECK-NEXT-L: q[3] = 5, status = 0
// CHECK-NEXT-L: q[4] = 6, status = 0
// CHECK-NEXT-L: q size 4
// CHECK-NEXT-L: q[0] = 3, status = 0
// CHECK-NEXT-L: q[1] = 4, status = 0
// CHECK-NEXT-L: q[2] = 5, status = 0
// CHECK-NEXT-L: q[3] = 6, status = 0
// CHECK-NEXT-L: q size 3
// CHECK-NEXT-L: q[0] = 3, status = 0
// CHECK-NEXT-L: q[1] = 5, status = 0
// CHECK-NEXT-L: q[2] = 6, status = 0
// CHECK-NEXT-L: q size 3
// CHECK-NEXT-L: q[0] = 3, status = 0
// CHECK-NEXT-L: q[1] = 5, status = 0
// CHECK-NEXT-L: q[2] = 6, status = 0
// CHECK-NEXT-L: q size 2
// CHECK-NEXT-L: q[0] = 3, status = 0
// CHECK-NEXT-L: q[1] = 5, status = 0
// CHECK-NEXT-L: q size 2
// CHECK-NEXT-L: q[0] = 3, status = 0
// CHECK-NEXT-L: q[1] = 5, status = 0
// CHECK-NEXT-L: q size 3
// CHECK-NEXT-L: q[0] = 4, status = 0
// CHECK-NEXT-L: q[1] = 3, status = 0
// CHECK-NEXT-L: q[2] = 5, status = 0
// CHECK-NEXT-L: q size 2
// CHECK-NEXT-L: q[0] = 3, status = 0
// CHECK-NEXT-L: q[1] = 5, status = 0

