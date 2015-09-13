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
#ifndef TERN_COMMON_SYNCFUNCS_H
#define TERN_COMMON_SYNCFUNCS_H

#include <cassert>
#include <limits.h>
#include <boost/static_assert.hpp>

namespace tern {
namespace syncfunc {

#undef DEF
#undef DEFTERNAUTO
#undef DEFTERNUSER

enum {
  not_sync = 0, // not a sync operation
# define DEF(func,kind,...) func,
# define DEFTERNAUTO(func)  func,
# define DEFTERNUSER(func)  func,
# include "syncfuncs.def.h"
# undef DEF
# undef DEFTERNAUTO
# undef DEFTERNUSER
  num_syncs,
  first_sync = 1,
  pthread_cond_wait_1 = 0x1000,
  pthread_cond_wait_2
};
BOOST_STATIC_ASSERT(num_syncs<USHRT_MAX);

extern const int kind[];
extern const char* name[];
extern const char* nameInTern[];

enum {Synchronization, BlockingSyscall, TernUser, TernAuto};

static inline bool isSync(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return kind[nr] == Synchronization;
}

static inline bool isBlockingSyscall(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return kind[nr] == BlockingSyscall;
}

static inline bool isTern(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return kind[nr] == TernUser || kind[nr] == TernAuto;
}

static inline bool isTernUser(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return kind[nr] == TernUser;
}

static inline bool isTernAuto(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return kind[nr] == TernAuto;
}

static inline const char* getName(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return name[nr];
}

static inline const char* getTernName(unsigned nr) {
  assert(first_sync <= nr && nr < num_syncs);
  return nameInTern[nr];
}

unsigned getNameID(const char* name);
unsigned getTernNameID(const char* nameInTern);

} // namespace syncfuncs

} // namespace tern

#endif
