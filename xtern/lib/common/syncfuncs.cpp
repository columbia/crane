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

/* Authors: Heming Cui (heming@cs.columbia.edu), Junfeng Yang (junfeng@cs.columbia.edu) */
#include <string.h>
#include "tern/syncfuncs.h"

#undef DEF
#undef DEFTERNAUTO
#undef DEFTERNUSER

namespace tern {
namespace syncfunc {

const int kind[] = {
  -1, // not_sync
# define DEF(func,kind,...) kind,
# define DEFTERNAUTO(func)  TernAuto,
# define DEFTERNUSER(func)  TernUser,
# include "tern/syncfuncs.def.h"
# undef DEF
# undef DEFTERNAUTO
# undef DEFTERNUSER
};

const char* name[] = {
  NULL, // not_sync
# define DEF(func,kind,...) #func,
# define DEFTERNAUTO(func)  #func,
# define DEFTERNUSER(func)  #func,
# include "tern/syncfuncs.def.h"
# undef DEF
# undef DEFTERNAUTO
# undef DEFTERNUSER
};

const char* nameInTern[] = {
  NULL, // not_sync
# define DEF(func,kind,...) "tern_"#func,
# define DEFTERNAUTO(func)  #func,
# define DEFTERNUSER(func)  #func"_real",
# include "tern/syncfuncs.def.h"
# undef DEF
# undef DEFTERNAUTO
# undef DEFTERNUSER
};

unsigned getNameID(const char* name) {
  assert(name && "got a null parameter.");
  for(unsigned i=first_sync; i<num_syncs; ++i)
    if(strcmp(name, getName(i)) == 0)
      return i;
  return not_sync;
}

unsigned getTernNameID(const char* name) {
  for(unsigned i=first_sync; i<num_syncs; ++i)
    if(strcmp(name, getTernName(i)) == 0)
      return i;
  return not_sync;
}

}
}
