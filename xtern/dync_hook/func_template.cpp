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


#ifndef __SPEC_HOOK_FUNC_NAME
extern "C" FUNC_RET_TYPE FUNC_NAME(ARGS_WITH_NAME){
  typedef FUNC_RET_TYPE (*orig_func_type)(ARGS_WITHOUT_NAME);

  static orig_func_type orig_func;

  void * handle;
  FUNC_RET_TYPE ret;
  void *eip = 0;

  if (!orig_func) {
    if(!(handle=dlopen("LIB_PATH", RTLD_LAZY))) {
      perror("dlopen");
      puts("here dlopen");
      abort();
    }

    orig_func = (orig_func_type) dlsym(handle, "FUNC_NAME");

    if(dlerror()) {
      perror("dlsym");
      puts("here dlsym");
      abort();
    }

    dlclose(handle);
  }

#ifdef __USE_TERN_RUNTIME
  if (Space::isApp()) {
    if (options::DMT) {
      //fprintf(stderr, "Parrot hook: pid %d self %u calls %s\n", getpid(), (unsigned)pthread_self(), "FUNC_NAME");
#ifdef __NEED_INPUT_INSID
      if (options::dync_geteip) {
        Space::enterSys();
        eip = get_eip();
        Space::exitSys();
      }
      record_rdtsc_op("FUNC_NAME", "START", 0, eip);
      ret = tern_FUNC_NAME((unsigned)(uint64_t) eip, ARGS_ONLY_NAME);
#else
      ret = tern_FUNC_NAME(ARGS_ONLY_NAME);
#endif
      record_rdtsc_op("FUNC_NAME", "END", 0, eip);
      return ret;
    } else {// For performance debugging, by doing this, we are still able to get the sync wait time for non-det mode.
      if (options::dync_geteip) {
        Space::enterSys();
        eip = get_eip();
        Space::exitSys();
      }
      record_rdtsc_op("FUNC_NAME", "START", 0, eip);
      Space::enterSys();
      ret = orig_func(ARGS_ONLY_NAME);
      Space::exitSys();
      record_rdtsc_op("FUNC_NAME", "END", 0, eip);
      return ret;
    }
  } 
#endif

  ret = orig_func(ARGS_ONLY_NAME);

  return ret;
}
#endif

