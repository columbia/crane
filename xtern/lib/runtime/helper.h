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

/* Authors: Heming Cui (heming@cs.columbia.edu), Junfeng Yang (junfeng@cs.columbia.edu) -*- Mode: C++ -*- */

#ifndef __TERN_COMMON_RUNTIME_HELPER_H
#define __TERN_COMMON_RUNTIME_HELPER_H

#include <pthread.h>

extern "C" {

  /* helper function used by tern runtimes.  It ensures a newly created
   * thread calls tern_task_begin() and tern_pthread_exit() */
  int __tern_pthread_create(pthread_t *thread,  const pthread_attr_t *attr,
                            void* (*start_routine)(void*), void *arg);

  /* tern inserts this helper function to the beginning of a program
   * (either as the first static constructor, or the first instruction in
   * main()).  This function schedules __tern_prog_end() and calls
   * tern_prog_begin() and tern_thread_begin() (for the main thread) */
  void __tern_prog_begin(void);

  /* calls tern_thread_end() (for the main thread) and tern_prog_end() */
  void __tern_prog_end(void);

  /* similar to tern_symbolic() except inserted by tern and checks for
   * valid arguments */
  //void __tern_symbolic(void *addr, int nbytes, const char *symname);

  /* mainly for marking arguments of main as symbolic */
  //void __tern_symbolic_argv(int argc, char **argv);
}

#endif
