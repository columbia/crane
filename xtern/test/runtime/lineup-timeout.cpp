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

// Test the timeout of lineup.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include "tern/user.h"

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
const long workload = 1e8;

void loop(const char *tag) {
  int loopCount;
  if (!strcmp(tag, "T0"))
    loopCount = 20;
  else if (!strcmp(tag, "T1"))
    loopCount = 16;
  else if (!strcmp(tag, "T2"))
    loopCount = 13;
  else
    loopCount = 10;

  for (int i = 0; i < loopCount; i++) { 
    pthread_mutex_lock(&mu);
    pthread_mutex_unlock(&mu);

    soba_wait(0);

    long sum = 0;
    for (long j = 0; j < workload; j++)
      sum += j*j;

    pthread_mutex_lock(&mu);
    printf("%s (%d) sum: %ld\n", tag, i, sum);
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

// Lineup 4 threads.
// CHECK-L: T3 (0) sum: 
// CHECK-NEXT-L: T0 (0) sum: 
// CHECK-NEXT-L: T1 (0) sum: 
// CHECK-NEXT-L: T2 (0) sum: 
// CHECK-NEXT-L: T2 (1) sum: 
// CHECK-NEXT-L: T3 (1) sum: 
// CHECK-NEXT-L: T0 (1) sum: 
// CHECK-NEXT-L: T1 (1) sum: 
// CHECK-NEXT-L: T1 (2) sum: 
// CHECK-NEXT-L: T2 (2) sum: 
// CHECK-NEXT-L: T3 (2) sum: 
// CHECK-NEXT-L: T0 (2) sum: 
// CHECK-NEXT-L: T0 (3) sum: 
// CHECK-NEXT-L: T1 (3) sum: 
// CHECK-NEXT-L: T2 (3) sum: 
// CHECK-NEXT-L: T3 (3) sum: 
// CHECK-NEXT-L: T3 (4) sum: 
// CHECK-NEXT-L: T0 (4) sum: 
// CHECK-NEXT-L: T1 (4) sum: 
// CHECK-NEXT-L: T2 (4) sum: 
// CHECK-NEXT-L: T2 (5) sum: 
// CHECK-NEXT-L: T3 (5) sum: 
// CHECK-NEXT-L: T0 (5) sum: 
// CHECK-NEXT-L: T1 (5) sum: 
// CHECK-NEXT-L: T1 (6) sum: 
// CHECK-NEXT-L: T2 (6) sum: 
// CHECK-NEXT-L: T3 (6) sum: 
// CHECK-NEXT-L: T0 (6) sum: 
// CHECK-NEXT-L: T0 (7) sum: 
// CHECK-NEXT-L: T1 (7) sum: 
// CHECK-NEXT-L: T2 (7) sum: 
// CHECK-NEXT-L: T3 (7) sum: 
// CHECK-NEXT-L: T3 (8) sum: 
// CHECK-NEXT-L: T0 (8) sum: 
// CHECK-NEXT-L: T1 (8) sum: 
// CHECK-NEXT-L: T2 (8) sum: 
// CHECK-NEXT-L: T2 (9) sum: 
// CHECK-NEXT-L: T3 (9) sum: 
// CHECK-NEXT-L: T0 (9) sum: 
// CHECK-NEXT-L: T1 (9) sum: 

// Lineup 3 threads.
// CHECK-NEXT-L: T2 (10) sum: 
// CHECK-NEXT-L: T0 (10) sum: 
// CHECK-NEXT-L: T1 (10) sum: 
// CHECK-NEXT-L: T2 (11) sum: 
// CHECK-NEXT-L: T0 (11) sum: 
// CHECK-NEXT-L: T1 (11) sum: 
// CHECK-NEXT-L: T2 (12) sum: 
// CHECK-NEXT-L: T0 (12) sum: 
// CHECK-NEXT-L: T1 (12) sum: 

// Lineup 2 threads.
// CHECK-NEXT-L: T0 (13) sum: 
// CHECK-NEXT-L: T1 (13) sum: 
// CHECK-NEXT-L: T0 (14) sum: 
// CHECK-NEXT-L: T1 (14) sum: 
// CHECK-NEXT-L: T0 (15) sum: 
// CHECK-NEXT-L: T1 (15) sum: 

// Only 1 thread.
// CHECK-NEXT-L: T0 (16) sum: 
// CHECK-NEXT-L: T0 (17) sum: 
// CHECK-NEXT-L: T0 (18) sum: 
// CHECK-NEXT-L: T0 (19) sum: 

