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

#ifndef __SPEC_HOOK_tern_lineup_init
extern "C" void soba_init(long opaque_type, unsigned count, unsigned timeout_turns){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_lineup_init_real(opaque_type, count, timeout_turns);
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_lineup_destroy
extern "C" void soba_destroy(long opaque_type){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_lineup_destroy_real(opaque_type);
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_lineup_start
extern "C" void tern_lineup_start(long opaque_type){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_lineup_start_real(opaque_type);
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_lineup_end
extern "C" void tern_lineup_end(long opaque_type){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_lineup_end_real(opaque_type);
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_lineup
extern "C" void soba_wait(long opaque_type){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_lineup_start_real(opaque_type);
    tern_lineup_end_real(opaque_type);
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_non_det_start
extern "C" void pcs_enter(){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations && options::enforce_non_det_annotations) {
    tern_non_det_start_real();
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_non_det_end
extern "C" void pcs_exit(){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations && options::enforce_non_det_annotations) {
    tern_non_det_end_real();
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_set_base_time
extern "C" void tern_set_base_timespec(struct timespec *ts){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_set_base_time_real(ts);
  } 
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_set_base_time
extern "C" void tern_set_base_timeval(struct timeval *tv){
#ifdef __USE_TERN_RUNTIME
  struct timespec ts;
  ts.tv_sec = tv->tv_sec;
  ts.tv_nsec = tv->tv_usec * 1000;
  if (Space::isApp() && options::DMT && options::enforce_annotations) {
    tern_set_base_time_real(&ts);
  }
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_detach
extern "C" void tern_detach(){
#ifdef __USE_TERN_RUNTIME
  /* This hint is for apache-like programs (a thread calls read() on a IPC pipe and never returns unless server is killed).
  So we do not need this options::enforce_annotations here (i.e., we must always enable it in Parrot for correctness). */
  if (Space::isApp() && options::DMT/* && options::enforce_annotations*/) {
    tern_detach_real();
  }
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_disable_sched_paxos
extern "C" void tern_disable_sched_paxos(){
#ifdef __USE_TERN_RUNTIME
  /* This hint is for apache-like programs (a thread calls read() on a IPC pipe and never returns unless server is killed).
  So we do not need this options::enforce_annotations here (i.e., we must always enable it in Parrot for correctness). */
  if (Space::isApp() && options::DMT/* && options::enforce_annotations*/) {
    tern_disable_sched_paxos_real();
  }
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_non_det_barrier_end
extern "C" void pcs_barrier_exit(int bar_id, int cnt){
#ifdef __USE_TERN_RUNTIME
  if (Space::isApp() && options::DMT && options::enforce_annotations && options::enforce_non_det_annotations) {
    tern_non_det_barrier_end_real(bar_id, cnt);
  }
#endif
  // If not runnning with xtern, NOP.
}
#endif

#ifndef __SPEC_HOOK_tern_init_affinity
static int num_thread_hint = 1;
extern "C" void tern_init_affinity(){
#ifdef __USE_TERN_RUNTIME
  /* This hint is just for init the affinity of threads, not an essential part of RepBox,
  but critical for getting stable evaluation performance results. */
  if (Space::isApp() && options::DMT/* && options::enforce_annotations*/) {
    num_thread_hint++; // Just hint, do not need lock.
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(num_thread_hint, &mask);
    assert (sched_setaffinity(0, sizeof(mask), &mask) == 0);
  }
#endif
  // If not runnning with xtern, NOP.
}
#endif

