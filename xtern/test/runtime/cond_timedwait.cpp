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

// FIXME: This program should be deterministic, but currently our 
// tern_prog_end() mechanism seems to have some small det issue, so make it   
// non-det temporarily.

#include <stdio.h>
#include "tern/user.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

pthread_mutex_t mu;
pthread_cond_t cv;

void* thread_func(void* arg) {
  struct timespec now;
  struct timespec next;
  clock_gettime(CLOCK_REALTIME, &now);
  tern_set_base_timespec(&now);
  next.tv_sec = now.tv_sec + 1; // timedwait for 1.234 second.
  next.tv_nsec = now.tv_nsec + 23400000;
  
  pthread_mutex_lock(&mu);
  pthread_cond_timedwait(&cv, &mu, &next);
  pthread_mutex_unlock(&mu);
}

int main(int argc, char *argv[], char* env[]) {
  int ret;
  const int nthreads = 1;
  pthread_t th[nthreads];

  pthread_mutex_init(&mu, NULL);
  pthread_cond_init(&cv, NULL);

  for (int i = 0; i < nthreads; i++) {
    ret  = pthread_create(&th[i], NULL, thread_func, NULL);
    assert(!ret && "pthread_create() failed!");
  }

  // Sleep for 3 seconds and signal().
  sleep(3);
  pthread_mutex_lock(&mu);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mu);
  printf("DONE.\n");
  
  for (int i = 0; i < nthreads; i++)
    pthread_join(th[i], NULL);

  return 0;
}

// CHECK: DONE.
