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

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "tern/user.h"

pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_barrier_t bar;

long sum = 0;

void delay() {
  //for (volatile int i = 0; i < 1e8; i++)
    //sum += i*i*i;
}

void *
thread(void *args)
{
  //assert(pthread_mutex_lock(&mutex) == 0);
  //printf("Critical section slave.\n");
  //assert(pthread_mutex_unlock(&mutex) == 0);
  pthread_barrier_wait(&bar);
  pcs_enter();
  int n = 0;
  fprintf(stderr, "\n\n\n\n\n\n\n\n\n============== start non-det self %u  =====================\n\n", (unsigned)pthread_self());
  delay();

  //assert(pthread_mutex_lock(&mutex2) == 0);
  fprintf(stderr, "Critical section slave2.\n");
  //assert(pthread_mutex_unlock(&mutex2) == 0);

  delay();

  fprintf(stderr, "\n\n============== end non-det self %u , sum %ld =====================\n\n\n\n\n\n\n\n\n\n", (unsigned)pthread_self(), sum);
  pcs_exit();

	for (int i = 0; i < 1e1; i++) {
      assert(pthread_mutex_lock(&mutex) == 0);
      fprintf(stderr, "Critical section slave3 %d.\n", i);
      assert(pthread_mutex_unlock(&mutex) == 0);
  }

  //pthread_exit(0);
  return NULL;
}

int 
main(int argc, char *argv[])
{
  const int nt = 4;
  //return 0;
  int i;
  pthread_t tid[nt];
  assert(pthread_mutex_init(&mutex,NULL) == 0);
  pthread_barrier_init(&bar, NULL, nt+1);

  pcs_enter();
  assert(pthread_mutex_init(&mutex2,NULL) == 0);
  pcs_exit();
  for (i = 0; i < nt; i++)
    assert(pthread_create(&tid[i],NULL,thread,NULL) == 0);

  pthread_barrier_wait(&bar);


  usleep(1);
	for (i = 0; i < 1e3; i++) {
      assert(pthread_mutex_lock(&mutex) == 0);
      fprintf(stderr, "Critical section master %d.\n", i);
      assert(pthread_mutex_unlock(&mutex) == 0);
  }

  for (i = 0; i < nt; i++)
    assert(pthread_join(tid[i], NULL) == 0);

  assert(pthread_mutex_destroy(&mutex) == 0);

  return 0;
}

