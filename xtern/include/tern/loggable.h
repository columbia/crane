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

#ifndef __TERN_RECORDER_LOGGABLE_H
#define __TERN_RECORDER_LOGGABLE_H

#include <vector>
#include "llvm/Instruction.h"

namespace tern {

/// NotLogged: instruction (or function body) is not logged at all
///
/// Logged: load or store instructions, calls to external non-sync or
///         summarized functions
//
/// LogSync: synchronization calls, logged, but the instrumentation is
///         handled by syncinstr, not loginstr, because replayer also
///         needs it
///
/// LogBBMarker: for marking instructions that purely for telling the
///         instruction log builder that a basic block is executed.  See
///         comments in instLogged() in loggable.cpp.  LogBBMarker can be
///         omitted if we use more sophisticated reconstruction algorithm.
///
enum LogTag {NotLogged, Logged, LogSync, LogBBMarker};

// LogTag getInstLoggedMD(const llvm::Instruction *I);
// void setInstLoggedMD(llvm::LLVMContext &C, llvm::Instruction *I, LogTag tag);

/// should we log the instruction?  can return all four LogTags
LogTag instLogged(llvm::Instruction *ins);

/// should we log instructions in @func?  return value can only be
/// NotLogged or Logged (i.e., can't be LogSync or LogBBMarker)
LogTag funcBodyLogged(llvm::Function *func);

/// Should we log calls to @func?  return value can only be NotLogged,
/// Logged, or LogSync (i.e., can't be LogBBMarker)
LogTag funcCallLogged(llvm::Function *func);

/// should we log this call ?  return value can only be NotLogged or
/// Logged (i.e., can't be LogBBMarker)
LogTag callLogged(llvm::Instruction *call);

}

#endif
