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

#include "tern/runtime/rdtsc.h"

using namespace std;

pthread_spinlock_t rdtsc_lock;
const size_t max_v_size = 300000000;
std::vector<sync_op_entry *> rdtsc_log;
size_t rdtsc_index = (size_t)-1;

void process_rdtsc_log(void) {
#if 0
  fprintf(stderr, "Storing rdtsc log...\n");fflush(stderr);
  char log_path[1024];
  memset(log_path, 0, 1024);
  snprintf(log_path, 1024, "%s/pself-pid-%u.txt", options::rdtsc_output_dir.c_str(), (unsigned)getpid());
  mkdir(options::rdtsc_output_dir.c_str(), 0777);
  FILE *f = fopen(log_path, "a+");
  assert(f);

  const char *opdeps[3] = {"", "----", "--------"};

  for (size_t i = 0; i < rdtsc_index; i++) {
    struct sync_op_entry *entry = rdtsc_log[i];
    assert(entry->op_print_depth < 3);
    fprintf(f, "%u %s%s %s %llu %p\n", entry->tid, opdeps[entry->op_print_depth], entry->op, entry->op_suffix, entry->clock, entry->eip);
    delete entry;
  }
  rdtsc_log.clear();
  fflush(f);
  fclose(f);
  rdtsc_index = 0;
#endif
}

void record_rdtsc_op(const char *op_name, const char *op_suffix, int print_depth, void *eip) {
  if (options::record_rdtsc) {
    if (rdtsc_index == (size_t)-1) {
      pthread_spin_init(&rdtsc_lock, 0);
      //printf("At exit...\n");
      atexit(process_rdtsc_log);
      rdtsc_log.reserve (max_v_size);
      rdtsc_index = 0;
    }

    struct sync_op_entry *entry = new struct sync_op_entry;    
    entry->tid = (unsigned)pthread_self();
    
    entry->op = op_name;
    entry->op_suffix = op_suffix;
    entry->op_print_depth = (unsigned)print_depth;
    entry->eip = eip;

    pthread_spin_lock(&rdtsc_lock);
    //assert(rdtsc_index < max_v_size && "Please pre-allocated a bigger vector size for rdtsc log.");
    if (rdtsc_index%1000000 == 0)
      fprintf(stderr, "rdtsc_index %u\n", (unsigned)rdtsc_index);
    if (rdtsc_index >= max_v_size) {
      process_rdtsc_log();
    }
    entry->clock = rdtsc(); // Put this into critical section, so the log is sorted.
    rdtsc_log[rdtsc_index] = entry;
    rdtsc_index++;
    pthread_spin_unlock(&rdtsc_lock);
  }
  //fprintf(stderr, "%u : %s %s %llu\n", (unsigned)pthread_self(), op_name, op_suffix, rdtsc());
}
