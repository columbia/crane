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

// Lineup the computation blocks which are serialized by the xtern RR 
// scheduler. The overhead with xtern now is very little after adding this lineup.

#include <stdio.h>
#include "tern/user.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <semaphore.h>

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
const long workload = 1e8;

void loop(const char *tag) {
  const int loopCount = 10;
  for (int i = 0; i < loopCount; i++) { 
    pthread_mutex_lock(&mu);
    pthread_mutex_unlock(&mu);

    soba_wait(0);   

    long sum = 0;
    for (long j = 0; j < workload; j++)
      sum += j*j;

    pthread_mutex_lock(&mu);
    printf("%s sum: %ld\n", tag, sum);
    pthread_mutex_unlock(&mu);
  }
}

void* thread_func(void* arg) {
  loop((const char *)arg);
}

int main(int argc, char *argv[], char* env[]) {
  int ret;
  const int nthreads = 4;
  pthread_t th[nthreads];
  const char *tags[nthreads] = {"T0", "T1", "T2", "T3"};

  soba_init(0, nthreads, 20);

  for (int i = 0; i < nthreads; i++) {
    ret  = pthread_create(&th[i], NULL, thread_func, (void *)tags[i]);
    assert(!ret && "pthread_create() failed!");
  }
  for (int i = 0; i < nthreads; i++)
    pthread_join(th[i], NULL);

  return 0;
}

// CHECK: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T2 sum: 
// CHECK-NEXT: T3 sum: 
// CHECK-NEXT: T0 sum: 
// CHECK-NEXT: T1 sum: 
