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

#ifndef __SPEC_HOOK_fcntl
extern "C" int fcntl(int fd, int cmd, ...){
  typedef int (*orig_func_type)(int , int , ...);
  static orig_func_type orig_func;

  va_list arg_list;
  va_start (arg_list, cmd);
  void *arg = va_arg (arg_list, void *);
  va_end (arg_list);

  void * handle;
  int ret;
  void *eip = 0;

  if (!orig_func) {
    if(!(handle=dlopen("/lib/x86_64-linux-gnu/libc.so.6", RTLD_LAZY))) {
      perror("dlopen");
      puts("here dlopen");
      abort();
    }

    orig_func = (orig_func_type) dlsym(handle, "fcntl");

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
#ifdef __NEED_INPUT_INSID
      if (options::dync_geteip) {
        Space::enterSys();
        eip = get_eip();
        Space::exitSys();
      }
      record_rdtsc_op("fcntl", "START", 0, eip);
      ret = tern_fcntl((unsigned)(uint64_t) eip, fd, cmd, arg);
#else
      ret = tern_fcntl(fd, cmd, arg);
#endif
      record_rdtsc_op("fcntl", "END", 0, eip);
      return ret;
    } else {// For performance debugging, by doing this, we are still able to get the sync wait time for non-det mode.
      if (options::dync_geteip) {
        Space::enterSys();
        eip = get_eip();
        Space::exitSys();
      }
      record_rdtsc_op("fcntl", "START", 0, eip);
      Space::enterSys();
      ret = orig_func(fd, cmd, arg);
      Space::exitSys();
      record_rdtsc_op("fcntl", "END", 0, eip);
      return ret;
    }
  } 
#endif

  ret = orig_func(fd, cmd, arg);

  return ret;
}
#endif
