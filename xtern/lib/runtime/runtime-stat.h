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

#ifndef __TERN_COMMON_RUNTIME_STAT_H
#define __TERN_COMMON_RUNTIME_STAT_H

#include <stdio.h>
#include <iostream>

namespace tern {
class RuntimeStat {
public:
  long nDetPthreadSyncOp; /* Number of deterministic pthread sync operations called (excluded idle thread and non-det sync operations).*/
  long nInterProcSyncOp;/* Number of inter-process sync operations called (networks, signals, wait, fork is scheduled by us and counted as nDetPthreadSyncOp).*/
  long nLineupSucc; /* Number of successful lineup operations (if multiple threads lineup and succeed for once, count as 1). */
  long nLineupTimeout; /* Number of lineup timeouts. */
  long nNonDetRegions;  /* Number of times all threads entering the non-det regions (and exiting the regions must be the same value). */
  long nNonDetPthreadSync; /* Number of non-det pthread sync operations called within a non-det region. */
  
public:
  RuntimeStat() {
    nDetPthreadSyncOp = 0;
    nInterProcSyncOp = 0;
    nLineupSucc = 0;
    nLineupTimeout = 0;
    nNonDetRegions = 0;
    nNonDetPthreadSync = 0;    
  }
  void print() {
    std::cout << "\n\nRuntimeStat:\n"
      << "nDetPthreadSyncOp\t" << "nInterProcSyncOp\t" << "nLineupSucc\t" << "nLineupTimeout\t" << "nNonDetRegions\t" << "nNonDetPthreadSync\t" << "\n"    
      << "RUNTIME_STAT: "
      << nDetPthreadSyncOp << "\t" << nInterProcSyncOp << "\t" << nLineupSucc << "\t" << nLineupTimeout << "\t" << nNonDetRegions << "\t" << nNonDetPthreadSync
      << "\n\n" << std::flush;
  }

};
}

#endif

