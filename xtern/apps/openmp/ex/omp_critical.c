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
   OpenMP example program which computes the dot product of two arrays a and b
   (that is sum(a[i]*b[i]) ) using explicit synchronization with a critical region.
   Compile with gcc -O3 -fopenmp omp_critical.c -o omp_critical
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N 100

int main (int argc, char *argv[]) {
  
  double a[N], b[N];
  double localsum, sum = 0.0;
  int i, tid, nthreads;
  
#pragma omp parallel shared(a,b,sum) private(i, localsum)
  {
    /* Get thread number */
    tid = omp_get_thread_num();
    
    /* Only master thread does this */
#pragma omp master
    {
      nthreads = omp_get_num_threads();
      printf("Number of threads = %d\n", nthreads);
    }

    /* Initialization */
#pragma omp for
    for (i=0; i < N; i++)
      a[i] = b[i] = (double)i;

    localsum = 0.0;

    /* Compute the local sums of all products */
#pragma omp for
    for (i=0; i < N; i++)
      localsum = localsum + (a[i] * b[i]);

#pragma omp critical
    sum = sum+localsum;

  }  /* End of parallel region */

  printf("   Sum = %2.1f\n",sum);
  exit(0);
}
