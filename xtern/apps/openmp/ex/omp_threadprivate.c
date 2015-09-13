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
  OpenMP example program demonstrating threadprivate variables
  Compile with: gcc -O3 -fopenmp omp_threadprivate.c -o omp_threadprivate
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int a, b, i, x, y, z, tid;
#pragma omp threadprivate(a,x, z)  /* a, x and z are threadprivate */


int main (int argc, char *argv[]) {

  /* Initialize the variables */
  a = b = x = y = z = 0;

  /* Fork a team of threads */
#pragma omp parallel private(b, tid) 
  {
    tid = omp_get_thread_num();
    a = b = tid;     /* a and b gets the value of the thread id */
    x = tid+10;      /* x is 10 plus the value of the thread id */
  }
  
  /* This section is now executed serially */
  for (i=0; i< 1000; i++) {
    y += i;
  }
  z = 40;  /* Initialize z outside the parallel region */
  

  /* Fork a new team of threads and initialize the threadprivate variable z */
#pragma omp parallel private(tid) copyin(z)
  {
    tid = omp_get_thread_num();
    z = z+tid;
    /* The variables a and x will keep their values from the last 
       parallel region but b will not. z will be initialized to 40 */
    printf("Thread %d:  a = %d  b = %d  x = %d z = %d\n", tid, a, b, x, z);
  }  /* All threads join master thread */

  exit(0);
}

