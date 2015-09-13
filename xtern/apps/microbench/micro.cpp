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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#if defined(ENABLE_DMP)
  #include "dmp.h"
#else
  #include <pthread.h>
#endif

#include "tern/user.h"

#define MAX (100)

int T; // number of threads
int C; // computation size
int I = 1E7; // total number of iterations

pthread_t th[MAX];
pthread_mutex_t mu[MAX];

int compute(int C) {
  int x = 0;
  for(int i=0;i<C;++i)
    x = x * i;
  return x;
}

void* thread_func(void* arg) {
  long tid = (long)arg;
  for(int i=0; i<I; ++i) {
    pthread_mutex_lock(&mu[tid]);
    pthread_mutex_unlock(&mu[tid]);

    compute(C);
  }
}

extern "C" int main(int argc, char * argv[]);
int main(int argc, char *argv[]) {
  int ret;

  assert(argc == 3);
  T = atoi(argv[1]); assert(T <= MAX);
  C = atoi(argv[2]);

  for(long i=0; i<T; ++i) {
    pthread_mutex_init(&mu[i], NULL);
    ret  = pthread_create(&th[i], NULL, thread_func, (void*)i);
    assert(!ret && "pthread_create() failed!");
  }
  for(int i=0; i<T; ++i)
    pthread_join(th[i], NULL);
  return 0;
}
