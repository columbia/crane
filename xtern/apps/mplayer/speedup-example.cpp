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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "tern/user.h"

#define N 8
#define M 10000

int nwait = 0;
int nexit = 0;
volatile long long sum;
long loops = 6e3;
pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_barrier_t bar;

void set_affinity(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  assert(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0);
}

void* thread_func(void *arg) {
  set_affinity((int)(long)arg);
  for (int j = 0; j < M; j++) {
    pthread_mutex_lock(&mutex);
    nwait++;
    for (long i = 0; i < loops; i++) // This is the key of speedup for parrot: the mutex needs to be a little bit congested.
      sum += i;
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    soba_wait(0);
    pthread_barrier_wait(&bar);
    for (long i = 0; i < loops; i++)
      sum += i*i*i*i*i*i;
    //fprintf(stderr, "compute thread %u %d\n", (unsigned)thread, sched_getcpu());
  }
}

int main(int argc, char *argv[]) {
  set_affinity(23);
  soba_init(0, N, 20);
  pthread_t th[N];
  int ret;
  pthread_cond_init(&cond, NULL);
  pthread_barrier_init(&bar, NULL, N);

  for(unsigned i=0; i<N; ++i) {
    ret  = pthread_create(&th[i], NULL, thread_func, (void*)i);
    assert(!ret && "pthread_create() failed!");
  }

  for (int j = 0; j < M; j++) {
    while (nwait < N) {
      sched_yield();
    }
    pthread_mutex_lock(&mutex);
    nwait = 0;
    //fprintf(stderr, "broadcast %u %d\n", (unsigned)pthread_self(), sched_getcpu());
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
  }

  for(unsigned i=0; i<N; ++i)
    pthread_join(th[i], NULL);

  exit(0);
}
