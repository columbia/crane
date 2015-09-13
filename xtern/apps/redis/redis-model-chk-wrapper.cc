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

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  int client_pid = fork();
  if (client_pid == 0) {
    //char *argv[] = { "/home/heming/rcs/smt+mc/xtern/apps/redis/redis-benchmark", "-n", "2", "-c", "2", "-t", "get" , NULL };
    char *argv[] = { "/home/jsimsa/smt+mc/xtern/apps/redis/redis-benchmark", "-n", "2", "-t", "lrange_100" , NULL };
    execv("/home/heming/rcs/smt+mc/xtern/apps/redis/redis-benchmark", argv);
  }
  sleep(0);
  int server_pid = fork();
  if (server_pid == 0) {
    char *argv[] = { "/home/heming/rcs/smt+mc/xtern/apps/redis/redis-server", NULL };
    execv("/home/heming/rcs/smt+mc/xtern/apps/redis/redis-server", argv);
  }
  waitpid(client_pid, NULL, 0);
  kill(server_pid, SIGTERM);
  waitpid(server_pid, NULL, 0);
  return 0;
}

