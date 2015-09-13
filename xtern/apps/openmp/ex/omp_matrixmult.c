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
  OpenMP implementation of matrix multiplication. Each thread takes care
  a chunk of rows. 

  Compile with gcc -O3 -fopenmp omp_matrixmult.c -o omp_matrixmult
*/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

/* Number of threads used */
#define NR_THREADS 4

/*
#define DEBUG 0

#define NRA 1400                 // number of rows in matrix A 
#define NCA 1400                 // number of columns in matrix A
#define NCB 1400                 // number of columns in matrix B
*/

/* Use smaller matrices for testing and debugging */
#define DEBUG 1
#define NRA 10                 // number of rows in matrix A
#define NCA 10                 // number of columns in matrix A
#define NCB 10                 // number of columns in matrix B


int main (int argc, char *argv[]) {
  int	tid, nthreads, i, j, k;
  double **a, **b, **c;
  double *a_block, *b_block, *c_block;
  double **res;
  double *res_block;
  double starttime, stoptime;

  a = (double **) malloc(NRA*sizeof(double *)); /* matrix a to be multiplied */
  b = (double **) malloc(NCA*sizeof(double *)); /* matrix b to be multiplied */
  c = (double **) malloc(NRA*sizeof(double *)); /* result matrix c */

  a_block = (double *) malloc(NRA*NCA*sizeof(double)); /* Storage for matrices */
  b_block = (double *) malloc(NCA*NCB*sizeof(double));
  c_block = (double *) malloc(NRA*NCB*sizeof(double));

  /* Result matrix for the sequential algorithm */
  res = (double **) malloc(NRA*sizeof(double *));
  res_block = (double *) malloc(NRA*NCB*sizeof(double));

  for (i=0; i<NRA; i++)   /* Initialize pointers to a */
    a[i] = a_block+i*NRA;

  for (i=0; i<NCA; i++)   /* Initialize pointers to b */
    b[i] = b_block+i*NCA;
  
  for (i=0; i<NRA; i++)   /* Initialize pointers to c */
    c[i] = c_block+i*NRA;

  for (i=0; i<NRA; i++)   /* Initialize pointers to res */
    res[i] = res_block+i*NRA;

  /* A static allocation of the matrices would be done like this */
  /* double a[NRA][NCA], b[NCA][NCB], c[NRA][NCB];  */

  /*** Spawn a parallel region explicitly scoping all variables ***/
#pragma omp parallel shared(a,b,c,nthreads) private(tid,i,j,k) num_threads(NR_THREADS)
  {
    tid = omp_get_thread_num();
    if (tid == 0) {  /* Only thread 0 prints */
      nthreads = omp_get_num_threads();
      printf("Starting matrix multiplication with %d threads\n",nthreads);
      printf("Initializing matrices...\n");
    }
    /*** Initialize matrices ***/
#pragma omp for nowait    /* No need to synchronize the threads before the */
    for (i=0; i<NRA; i++) /* last matrix has been initialized */
      for (j=0; j<NCA; j++)
	a[i][j]= (double) (i+j);
#pragma omp for nowait
    for (i=0; i<NCA; i++)
      for (j=0; j<NCB; j++)
	b[i][j]= (double) (i*j);
#pragma omp for   /* We synchronize the threads after this */
    for (i=0; i<NRA; i++)
      for (j=0; j<NCB; j++)
	c[i][j]= 0.0;

    if (tid == 0) /* Thread zero measures time */
      starttime = omp_get_wtime();  /* Master thread measures the execution time */
    
    /* Do matrix multiply sharing iterations on outer loop */
    /* If DEBUG is TRUE display who does which iterations */
    /* printf("Thread %d starting matrix multiply...\n",tid); */
#pragma omp for
    for (i=0; i<NRA; i++) {
      if (DEBUG) printf("Thread=%d did row=%d\n",tid,i);
      for(j=0; j<NCB; j++) {    
	for (k=0; k<NCA; k++) {
	  c[i][j] += a[i][k] * b[k][j];
	}
      }
    }

    if (tid == 0) {
      stoptime = omp_get_wtime();
      printf("Time for parallel matrix multiplication: %3.2f s\n", 
	     stoptime-starttime);
    }
  }   /*** End of parallel region ***/
  
  starttime = omp_get_wtime();
  /* Do a sequential matrix multiplication and compare the results */
  for (i=0; i<NRA; i++) {
    for (j=0; j<NCB; j++) {
      res[i][j] = 0.0;
      for (k=0; k<NCA; k++)
	res[i][j] += a[i][k]*b[k][j];
    }
  }
  stoptime = omp_get_wtime();
  printf("Time for sequential matrix multiplication: %3.2f s\n", stoptime-starttime);

  /* Check that the results are the same as in the parallel solution.
     Actually, you should not compare floating point values for equality like this
     but instead compute the difference between the two values and check that it
     is smaller than a very small value epsilon. However, since all values in the
     matrices here are integer values, this will work.
  */
  for (i=0; i<NRA; i++) {
    for (j=0; j<NCB; j++) {
      if (res[i][j] == c[i][j]) {
	/* Everything is OK if they are equal */
      }
      else {
	printf("Different result %5.1f != %5.1f in %d %d\n ", res[i][j], c[i][j], i, j);
      }
    }
  }

  /* If DEBUG is true, print the results. Usa smaller matrices for this */
  if (DEBUG) {
    printf("Result Matrix:\n");
    for (i=0; i<NRA; i++) {
      for (j=0; j<NCB; j++) 
	printf("%6.1f ", c[i][j]);
      printf("\n"); 
    }
  }

  printf ("Done.\n");
  exit(0);
}
