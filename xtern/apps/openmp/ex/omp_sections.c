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
  OpenMP example program demonstrating the use of the sections construct
  Compile with: gcc -fopenmp omp_sections.c -o omp_sections
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define N     50

int main (int argc, char *argv[]) {
  int i, nthreads, tid;
  float a[N], b[N], c[N], d[N];
  
  /* Some initializations */
  for (i=0; i<N; i++) {
    a[i] = i * 1.5;
    b[i] = i + 42.0;
    c[i] = d[i] = 0.0;
  }

  /* Start 2 threads */
#pragma omp parallel shared(a,b,c,d,nthreads) private(i,tid) num_threads(2)
  {
    tid = omp_get_thread_num();
    if (tid == 0) {
      nthreads = omp_get_num_threads();
      printf("Number of threads = %d\n", nthreads);
    }
    printf("Thread %d starting...\n",tid);
    
#pragma omp sections
    {
#pragma omp section
      {
	
	printf("Thread %d doing section 1\n",tid);
	for (i=0; i<N; i++) {
	  c[i] = a[i] + b[i];
	}
	//sleep(tid+2);  /* Delay the thread for a few seconds */
	/* End of first section */
      }
#pragma omp section
      {
	printf("Thread %d doing section 2\n",tid);
	for (i=0; i<N; i++) {
	  d[i] = a[i] * b[i];
	}
	//sleep(tid+2);   /* Delay the thread for a few seconds */
      } /* End of second section */
      
    }  /* end of sections */
    
    printf("Thread %d done.\n",tid); 

  }  /* end of omp parallel */

  /* Print the results */
  printf("c:  ");
  for (i=0; i<N; i++) {
    printf("%.2f ", c[i]);
  }
  
  printf("\n\nd:  ");
  for (i=0; i<N; i++) {
    printf("%.2f ", d[i]);
  }
  printf("\n");

  exit(0);
}
