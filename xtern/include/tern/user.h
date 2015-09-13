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
#ifndef __TERN_COMMON_USER_H
#define __TERN_COMMON_USER_H

/* users manually insert these macros to their programs */

#ifdef __cplusplus
extern "C" {
#endif

  /// mark memory [@addr, @addr+@nbytes) as symbolic; @symname names this
  /// symbolic memory for debugging
  void tern_symbolic(void *addr, int nbytes, const char *symname);

  /// for server programs, users manually insert these two methods at
  /// beginning and the end of the processing of a user request.  @addr
  /// and @nbytes mark the relevant request data as symbolic.
  void tern_task_begin(void *addr, int nbytes, const char *name);
  void tern_task_end(void);

  /// New programming primitives to get better performance without affecting
  /// logics of applications.
  void soba_init(long opaque_type, unsigned count, unsigned timeout_turns);
  void soba_destroy(long opaque_type);
  void tern_lineup_start(long opaque_type);
  void tern_lineup_end(long opaque_type);
  void soba_wait(long opaque_type);

  void tern_workload_start(long opaque_type, unsigned workload_hint);
  void tern_workload_end(long opaque_type);

  void pcs_enter();
  void pcs_exit();
  void tern_detach();
  void tern_disable_sched_paxos();
  void pcs_barrier_exit(int bar_id, int cnt);
  void tern_init_affinity();

  /// Set thread local base time. This is for pthread_cond_timedwait(), sem_timedwait() and pthread_mutex_timedlock().
  void tern_set_base_timespec(struct timespec *ts);
  void tern_set_base_timeval(struct timeval *tv);

#ifdef __cplusplus
}
#endif

#endif
