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

#include "tern/user.h"

#ifdef __cplusplus
extern "C" {
#endif

void soba_init(long opaque_type, unsigned count, unsigned timeout_turns) {
  //fprintf(stderr, "Non-deterministic soba_init\n");
}

void soba_destroy(long opaque_type) {
  //fprintf(stderr, "Non-deterministic soba_destroy\n");
}

void tern_lineup_start(long opaque_type) {
  //fprintf(stderr, "Non-deterministic tern_lineup_start\n");
}

void tern_lineup_end(long opaque_type) {
  //fprintf(stderr, "Non-deterministic tern_lineup_end\n");
}

void soba_wait(long opaque_type) {
  //fprintf(stderr, "Non-deterministic soba_wait\n");
}

void pcs_enter() {
  //fprintf(stderr, "Non-deterministic pcs_enter\n");
}

void pcs_exit() {
  //fprintf(stderr, "Non-deterministic pcs_exit\n");
}

void tern_set_base_timespec(struct timespec *ts) {
  //fprintf(stderr, "Non-deterministic tern_set_base_timespec\n");
}

void tern_set_base_timeval(struct timeval *tv) {
  //fprintf(stderr, "Non-deterministic tern_set_base_timeval\n");
}

void tern_detach() {
  //fprintf(stderr, "Non-deterministic tern_detach\n");
}

void tern_disable_sched_paxos() {
  //fprintf(stderr, "Non-deterministic tern_disable_sched_paxos\n");
}

void pcs_barrier_exit(int bar_id, int cnt) {
  //fprintf(stderr, "Non-deterministic pcs_barrier_exit\n");
}

void tern_init_affinity() {
  //fprintf(stderr, "Non-deterministic tern_init_affinity\n");
}

#ifdef __cplusplus
}
#endif
