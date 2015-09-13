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
#ifndef __TERN_RECORDER_LOG_H
#define __TERN_RECORDER_LOG_H

#include <assert.h>
#include <stdarg.h>
#include <fstream>
#include <tr1/unordered_map>

#include "tern/logdefs.h"

namespace tern {

struct Logger {
  /// per-thread logging functions and data
  virtual void logInsid(unsigned insid) {}
  virtual void logLoad (unsigned insid, char* addr, uint64_t data) {}
  virtual void logStore(unsigned insid, char* addr, uint64_t data) {}
  virtual void logCall(uint8_t flags, unsigned insid,
                       short narg, void* func, va_list args) {}
  virtual void logRet(uint8_t flags, unsigned insid,
                      short narg, void* func, uint64_t data) {}
  virtual void logSync(unsigned insid, unsigned short sync,
                       unsigned turn, 
                       timespec time1, 
                       timespec time2, timespec sched_time, 
                       bool after = true, ...) {}
  virtual void flush() {}
  virtual ~Logger() {}
  static __thread Logger* the; /// pointer to per-thread logger

#if 0
  /// obsolete
  static void markFuncCallLogged(void *func, unsigned funcid, const char* name){
    funcsCallLogged[func] = funcid;
  }
  static void markFuncEscape(void *func, unsigned funcid, const char* name) {
    funcsEscape[func] = funcid;
  }
  static bool funcCallLogged(void *func) {
    return funcsCallLogged.find(func) != funcsCallLogged.end();
  }
  static bool funcEscape(void *func) {
    return funcsEscape.find(func) != funcsEscape.end();
  }

  static func_map funcsEscape;
#endif

  /// code and data shared by all loggers
  static void progBegin();
  static void progEnd();
  static void threadBegin(int tid);
  static void threadEnd(void);

  /// map function address at runtime to a unique function ID.  We need
  /// this mapping so that we can find the llvm::Function* corresponding
  /// to a callee when analyzing a log
  static void mapFuncToID(void *func, unsigned funcid, const char* name){
    funcs[func] = funcid;
  }
  static bool funcHasID(void *func) {
    return funcs.find(func) != funcs.end();
  }
  typedef std::tr1::unordered_map<void*, unsigned> func_map;
  static func_map funcs; /// function address at runtime to ID map
};

struct TxtLogger: public Logger {
  virtual void logSync(unsigned insid, unsigned short sync,
                       unsigned turn,
                       timespec time1, 
                       timespec time2, timespec sched_time, 
                       bool after = true, ...);
  TxtLogger(int tid);
  virtual ~TxtLogger();

protected:
  void print_header();

  int tid;
  std::fstream ouf;
};

struct BinLogger: public Logger {
  virtual void logInsid(unsigned insid);
  virtual void logLoad (unsigned insid, char* addr, uint64_t data);
  virtual void logStore(unsigned insid, char* addr, uint64_t data);
  virtual void logCall(uint8_t flags, unsigned insid,
                       short narg, void* func, va_list args);
  virtual void logRet(uint8_t flags, unsigned insid,
                      short narg, void* func, uint64_t data);
  virtual void logSync(unsigned insid, unsigned short sync,
                       unsigned turn, 
                       timespec time1, 
                       timespec time2, timespec sched_time, 
                       bool after = true, ...);
  virtual ~BinLogger();
  BinLogger(int tid);

protected:

  void mapLogTrunk(void);
  void checkAndGrowLogSize(void) {
    // TODO: check log buffer size and allocate new space if necessary
    assert(off + RECORD_SIZE <= TRUNK_SIZE);
  }

  char*      buf;
  unsigned   off;
  int        fd;
  off_t      foff;
};


/// logger for testing; prints out a canonicalized log that remains the
/// same across different deterministic runs.  Note that pointer addresses
/// are fine because our testing script canonicalizes them
struct TestLogger: public Logger {
  virtual void logSync(unsigned insid, unsigned short sync,
                       unsigned turn, 
                       timespec time1, 
                       timespec time2, timespec sched_time, 
                       bool after = true, ...);
  virtual void flush();
  TestLogger(int tid);
  virtual ~TestLogger();

protected:

  int tid;
  std::fstream ouf;
};


}

#endif
