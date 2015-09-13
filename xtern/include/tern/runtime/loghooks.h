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
#ifndef __TERN_RECORDER_LOGHOOKS_H
#define __TERN_RECORDER_LOGHOOKS_H

#include <stdint.h>

extern "C" {

  void tern_log_insid(unsigned insid);
  void tern_log_load (unsigned insid, char* addr, uint64_t data);
  void tern_log_store(unsigned insid, char* addr, uint64_t data);
  void tern_log_call(uint8_t flags, unsigned insid,
                     short narg, void* func, ...);
  void tern_log_ret(uint8_t flags, unsigned insid,
                    short narg, void* func, uint64_t ret);

  /// The following three functions are for connecting the llvm::Function
  /// objects we statically see within a bitcode module with the function
  /// addresses logged at runtime.
  ///
  /// Statically within LLVM, we only see LLVM::Function objects and know
  /// their names, but we don't know their addresses at runtime.  At
  /// runtime, the program knows only addresses and sees no llvm::Function
  /// or function names.
  ///
  /// We solve this problem by creating two mappings:
  /// (1) name    <-> integer ID, used at analysis time
  /// (2) address <-> integer ID, used at runtime for logging
  ///
  /// The instrumentor loginstr creates mapping (1) while instrumenting a
  /// bc module.  It also writes a tern_funcs_call_logged() fucntion that
  /// builds mapping (2) at runtime.
  ///
  /// This is a little bit gross ...
  ///
  void tern_funcs_call_logged(void); ///will be replaced by loginstr
  void tern_func_call_logged(void* func, unsigned funcid, const char* name);
  void tern_func_escape(void* func, unsigned funcid, const char* name);
}

#endif
