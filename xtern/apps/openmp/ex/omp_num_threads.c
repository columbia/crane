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

/*
  OpenMP example program demonstrating the use of the num_threads clause
  The master thread forks a parallel region with T threads.
  The value of T is given as an argument on the command line with the -t switch
  Compile with: gcc -O2 -fopenmp omp_num_threads.c -o omp_num_threads
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage(char* name) {
  printf("Usage: %s -t <nr of threads>\n", name);
  exit(0);
}

int main (int argc, char *argv[]) {
  
  int nthreads, tid;
  int T=2;   /* Default number of threads */
  char c;
  char name[50];

  /* Check that we have at least one argument */
  if (argc <=1 ) {
    print_usage(argv[0]);
  }
  else {
    /* Get the value of T from the command line */
    while ((c=getopt(argc, argv, "ht:")) != EOF) {
      switch (c) {
      case 'h':
	print_usage(argv[0]);
      case 't':
	T = atoi(optarg);    /* Get number of threads  */
	break;
      default: 
	print_usage(argv[0]);
      }
    }
  }
  
  /* Fork a team of T threads */
#pragma omp parallel private(nthreads, tid) num_threads(T)
  {
    /* Get thread number */
    tid = omp_get_thread_num();
    printf("Hello World from thread = %d\n", tid);
    
    /* Only master thread does this */
    if (tid == 0) {
      nthreads = omp_get_num_threads();
      gethostname(name, 50);   /* Get host name */
      printf("Number of threads on node %s = %d\n", name, nthreads);
    }
  }  /* All threads join master thread and disband */
  exit(0);  
}
