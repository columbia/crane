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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#ifdef PRINT_DEBUG
#  define dprintf(fmt...) fprintf(stderr, fmt)
#else
#  define dprintf(fmt...)
#endif

#ifdef __SPEC_HOOK___libc_start_main

extern "C" void __tern_prog_begin(void);  //  lib/runtime/helper.cpp
extern "C" void __tern_prog_end(void); //  lib/runtime/helper.cpp

typedef int (*main_type)(int, char**, char**);

struct arg_type
{
  char **argv;
  int (*main_func) (int, char **, char **);
};

main_type saved_init_func = NULL;
void tern_init_func(int argc, char **argv, char **env){
  dprintf("%04d: __tern_init_func() called.\n", (int) pthread_self());
  if(saved_init_func)
    saved_init_func(argc, argv, env);
  __tern_prog_begin();
}

typedef void (*fini_type)(void*);
fini_type saved_fini_func = NULL;
void tern_fini_func(void* status) {
  // note that fprintf may not print out anything here because (1) stderr
  // may be redirected and (2) we're close to program exit and the libc
  // data structures may already be cleared
  dprintf("%04d: __tern_fini_func() called.\n", (int) pthread_self());
  __tern_prog_end();
  if(saved_fini_func)
    saved_fini_func(status);
}

extern "C" int my_main(int argc, char **pt, char **aa)
{
  int ret;
  arg_type *args = (arg_type*)pt;
  dprintf("%04d: __libc_start_main() called.\n", (int) pthread_self());
  ret = args->main_func(argc, args->argv, aa);
  return ret;
}

extern "C" int __libc_start_main(
  void *func_ptr,
  int argc,
  char* argv[],
  void (*init_func)(void),
  void (*fini_func)(void),
  void (*rtld_fini_func)(void),
  void *stack_end)
{
  typedef void (*fnptr_type)(void);
  typedef int (*orig_func_type)(void *, int, char *[], fnptr_type,
                                fnptr_type, fnptr_type, void*);
  orig_func_type orig_func;
  arg_type args;

  void * handle;
  int ret;

  // Get lib path.
  Dl_info dli;
  dladdr((void *)dlsym, &dli);
  std::string libPath = dli.dli_fname;
  libPath = dli.dli_fname;
  size_t lastSlash = libPath.find_last_of("/");
  libPath = libPath.substr(0, lastSlash);
  libPath += "/libc.so.6";
  libPath = "/lib/x86_64-linux-gnu/libc.so.6"; // Heming hack: weird thing on bug03.

  if(!(handle=dlopen(libPath.c_str(), RTLD_LAZY))) {
    puts("dlopen error");
    abort();
  }

  orig_func = (orig_func_type) dlsym(handle, "__libc_start_main");

  if(dlerror()) {
    puts("dlerror");
    abort();
  }

  dlclose(handle);

#ifdef __USE_TERN_RUNTIME
  dprintf("%04d: __libc_start_main is hooked.\n", (int) pthread_self());
  //fprintf(stderr, "%04d: __tern_prog_begin() called.\n", (int) pthread_self());
  args.argv = argv;
  args.main_func = (main_type)func_ptr;
  saved_init_func = (main_type)init_func;
  // note that we don't hook fini_func because it appears that fini_func
  // is only called for statically linked libc, whereas rtld_fini_func is
  // called regardless
  saved_fini_func = (fini_type)rtld_fini_func;
  ret = orig_func((void*)my_main, argc, (char**)(&args),
                  (fnptr_type)tern_init_func, (fnptr_type)fini_func,
                  rtld_fini_func, stack_end);
                  //(fnptr_type)tern_fini_func, stack_end);
  return ret;
#endif
  ret = orig_func(func_ptr, argc, argv, init_func, fini_func,
                  rtld_fini_func, stack_end);

	return ret;
}
#endif

