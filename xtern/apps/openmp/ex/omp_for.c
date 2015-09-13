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
  OpenMP example program that demonstrates the for construct
  Compile with: gcc -O3 -fopenmp omp_for.c -o omp_for
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {
  
  int i, tid, nthreads, n=20;
  double *A, *B, *C, *D;

  /* Allocate the arrays */
  A = (double *) malloc(n*sizeof(double));
  B = (double *) malloc(n*sizeof(double));
  C = (double *) malloc(n*sizeof(double));
  D = (double *) malloc(n*sizeof(double));

  /* Initialize the arrays A and C */
  for (i=0; i<n; i++) {
    A[i] = (double)(i+1);
    C[i] = (double)(i+1);
  }

  /* Fork a team of threads */
#pragma omp parallel private(tid, i) shared(n,A,B,C,D)
  {
    tid = omp_get_thread_num();
    /* Only master thread does this */
    if (tid == 0) 
      {
	nthreads = omp_get_num_threads();
	printf("Number of threads = %d\n", nthreads);
      }

#pragma omp for nowait
    for (i=0; i<n; i++) {
      B[i] = 2.0*A[i];
    }

#pragma omp for
    /* #pragma omp for schedule(static, 2) */
    /* #pragma omp for schedule(guided, 2) */
    for (i=0; i<n; i++) {
      D[i] = 1.0/C[i];
      printf("Thread %d does iteration %d\n", tid, i);
    }
    
  }  /* End of parallel region */

  /* Print out the result */
  printf("B: ");
  for (i=0; i<n; i++) {
    printf("%3.2f ", B[i]);
  }
  printf("\n\nD: ");
  for (i=0; i<n; i++) {
    printf("%2.3f ", D[i]);
  }
  printf("\n");

  exit(0);
}

