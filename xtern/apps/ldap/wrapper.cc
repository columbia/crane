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
    char *argv[] = { "/home/heming/rcs/smt+mc/xtern/apps/ldap/slapd-mtread", "-H", "ldap://localhost:9011/", "-D", 
                     "cn=Manager,dc=example,dc=com", "-w", "secret", "-e", "cn=Monitor", "-c", "2", "-m", "2", "-L", "5", "-l", "10", NULL };
    execv("/home/heming/rcs/smt+mc/xtern/apps/ldap/slapd-mtread", argv);
  }
  sleep(0);
  int server_pid = fork();
  if (server_pid == 0) {
    char *argv[] = { "/home/heming/rcs/smt+mc/xtern/apps/ldap/slapd", "-s0", "-f", "slapd.1.conf", "-h",
      "ldap://localhost:9011/", "-d", "stats", NULL };
    execv("/home/heming/rcs/smt+mc/xtern/apps/ldap/slapd", argv);
  }
  waitpid(client_pid, NULL, 0);
  kill(server_pid, SIGTERM);
  waitpid(server_pid, NULL, 0);
  return 0;
}

