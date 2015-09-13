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
  OpenMP example program that reads and prints out some environment variables
  that control OpenMP execution.
  Compile with gcc -O3 -fopenmp omp_get_env_info.c -o omp_get_env_info
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char *argv[]) 
{
  int nthreads, tid, procs, maxt, inpar, dynamic, nested;
  char name[50];
  
  /* Start parallel region */
#pragma omp parallel private(nthreads, tid)
  {
    
    /* Obtain thread number */
    tid = omp_get_thread_num();
    
    /* Only master thread does this
       We could also use #pragma omp master
     */
    if (tid == 0) 
      {
	printf("Thread %d getting environment info...\n", tid);
	
	/* Get host name */
	gethostname(name, 50);

	/* Get environment information */
	procs = omp_get_num_procs();
	nthreads = omp_get_num_threads();
	maxt = omp_get_max_threads();
	inpar = omp_in_parallel();
	dynamic = omp_get_dynamic();
	nested = omp_get_nested();
	
	/* Print environment information */
	printf("Hostname = %s\n", name);
	printf("Number of processors = %d\n", procs);
	printf("Number of threads = %d\n", nthreads);
	printf("Max threads = %d\n", maxt);
	printf("In parallel? = %d\n", inpar);
	printf("Dynamic threads enabled? = %d\n", dynamic);
	printf("Nested parallelism supported? = %d\n", nested);
	
      }
    
  }  /* Done */
  exit(0);
}

