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

/* Authors: Heming Cui (heming@cs.columbia.edu), Junfeng Yang (junfeng@cs.columbia.edu) */

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "tern/config.h"
#include "tern/hooks.h"
#include "tern/runtime/runtime.h"
#include "tern/space.h"
#include "helper.h"
#include "tern/options.h"

using namespace tern;
#define IDLE_MUTEX_INS 0xdeadbeaf

extern "C" {

/*
    This idle thread is created to avoid empty runq.
    In the implementation with semaphore, there's a global token that must be 
    held by some threads. In the case that all threads are executing blocking
    function calls, the global token can be held by nothing. So we create this
    idle thread to ensure there's at least one thread in the runq to hold the 
    global token.

    Another solution is to add a flag in schedule showing if it happens that 
    runq is empty and the global token is held by no one. And recover the global
    token when some thread comes back to runq from blocking function call. 
 */
volatile int idle_done = 0;
pthread_t idle_th;
pthread_mutex_t idle_mutex;
pthread_cond_t idle_cond;

typedef void * (*thread_func_t)(void*);
static void *__tern_thread_func(void *arg) {
  void **args;
  void *ret_val;
  thread_func_t user_thread_func;
  void *user_thread_arg;

  args = (void**)arg;
  user_thread_func = (thread_func_t)((intptr_t)args[0]);
  user_thread_arg = args[1];
  // free arg before calling user_thread_func as it may not return (i.e.,
  // it may call pthread_exit())
  delete[] (void**)arg;

  tern_thread_begin();
  ret_val = user_thread_func(user_thread_arg);
  tern_pthread_exit(-1, ret_val); // calls tern_thread_end() and pthread_exit()
  assert(0 && "unreachable!");
}

int __tern_pthread_create(pthread_t *thread,  const pthread_attr_t *attr,
                          thread_func_t user_thread_func,
                          void *user_thread_arg) {
  void **args;

  // use heap because stack of this func may be deallocated before the
  // created thread reads the @args
  args = new void*[2];
  args[0] = (void*)(intptr_t)user_thread_func;
  args[1] = user_thread_arg;

  int ret;
  ret = Runtime::__pthread_create(thread, const_cast<pthread_attr_t*>(attr), __tern_thread_func, (void*)args);
  if(ret < 0)
    delete[] (void**)args; // clean up memory for @args
  return ret;
}

void *idle_thread(void *)
{
  while (true) {
    //fprintf(stderr, "idle thread...\n");
    tern_pthread_mutex_lock(IDLE_MUTEX_INS, &idle_mutex);
    if (!idle_done) {
      tern_idle_cond_wait();
    } else {
      tern_pthread_mutex_unlock(IDLE_MUTEX_INS, &idle_mutex);
      break;
    }
    tern_pthread_mutex_unlock(IDLE_MUTEX_INS, &idle_mutex);
    tern_idle_sleep();
  }
  return NULL;
}

static pthread_t main_thread_th;
static bool prog_began = false; // sanity
//  SYS -> SYS
//  must be called by the main thread
void __tern_prog_begin(void) {
  assert(!prog_began && "__tern_prog_begin() already called!");
  prog_began = true;

  //fprintf(stderr, "%08d calls __tern_prog_begin\n", (int) pthread_self());
  assert(Space::isSys() && "__tern_prog_begin must start in sys space");

  main_thread_th = pthread_self();

  options::read_options("local.options");
  options::read_env_options();
  options::print_options("dump.options");
  tern::InstallRuntime();
  // FIXME: the version of uclibc in klee doesn't seem to pick up the
  // functions registered with atexit()
  atexit(__tern_prog_end);

  tern_prog_begin();
  assert(Space::isSys());
  tern_thread_begin(); // main thread begins
  assert(Space::isApp());

  //  use tern_pthread_create because we want to fake the eip
  if (options::launch_idle_thread)
  {
    tern_pthread_mutex_init(IDLE_MUTEX_INS, &idle_mutex, NULL);
    tern_pthread_create(0xdead0000, &idle_th, NULL, idle_thread, NULL);
  }
  assert(Space::isApp() && "__tern_prog_begin must end in app space");
}

//  SYS -> SYS
void __tern_prog_end (void) {
  assert(prog_began && "__tern_prog_begin() not called "\
         "or __tern_prog_end() already called!");
  prog_began = false;
  assert(Space::isApp() && "__tern_prog_end must start in app space");
  tern_print_runtime_stat();

  // terminate the idle thread because it references the runtime which we
  // are about to free
  tern_pthread_mutex_lock(IDLE_MUTEX_INS, &idle_mutex);
  idle_done = 1;
  tern_pthread_mutex_unlock(IDLE_MUTEX_INS, &idle_mutex);
  tern_pthread_cond_signal(IDLE_MUTEX_INS, &idle_cond);

  Space::enterSys();
  pthread_mutex_lock(&idle_mutex);
  pthread_cond_signal(&idle_cond);
  pthread_mutex_unlock(&idle_mutex);
  Space::exitSys();
  
  //  use tern_pthread_join because we want to fake the eip
  if (options::launch_idle_thread)
  {
    assert(pthread_self() != idle_th && "idle_th should never reach __tern_prog_end");
    tern_pthread_join(0xdeadffff, idle_th, NULL);
  }
  tern_thread_end(-1); // main thread ends

  assert(Space::isSys());
  tern_prog_end();

  // Mut not delete the runtime, because at this moment there could be still child 
  // threads running yet, e.g., OpenMP. If delete this, will have seg fault sometimes.
  //delete tern::Runtime::the;
  //tern::Runtime::the = NULL;
  assert(Space::isSys() && "__tern_prog_end must end in system space");
}

void __tern_symbolic(unsigned insid, void *addr,
                     int nbytes, const char *symname) {
  if(nbytes <= 0 || !addr)
    return;
  tern_symbolic_real(insid, addr, nbytes, symname);
}

void __tern_symbolic_argv(unsigned insid, int argc, char **argv) {
  char arg[32];
  for(int i=0; i<argc; ++i) {
    sprintf(arg, "arg%d\n", i);
    tern_symbolic_real(insid, argv[i], strlen(argv[i])+1, arg);
  }
}

}
