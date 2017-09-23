/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
  int i, j, ii, jj;
  int t0, t1, t2, t3, t4, t5, t6, t7;

  /* Transpose 32x32 matrix.
   * 
   * Using blocking to increase temporal locality:
   * Partition matrix into sixteen(4x4) 8x8 submatrices, then transpose each
   * of them.
   * To deal with conflict misses along the matrix diagonal, use local
   * variables (registers) to cache them.
   */
  if (N == 32 && M == 32) {
    for (ii = 0; ii < N; ii += 8) {
      for (jj = 0; jj < M; jj += 8) {
        /* To deal with conflict misses along the diagonal,
         * load and store one cache line each time. */
        for (i = 0; i < 8; i++) {
          t0 = A[ii+i][jj];
          t1 = A[ii+i][jj+1];
          t2 = A[ii+i][jj+2];
          t3 = A[ii+i][jj+3];
          t4 = A[ii+i][jj+4];
          t5 = A[ii+i][jj+5];
          t6 = A[ii+i][jj+6];
          t7 = A[ii+i][jj+7];

          B[jj][ii+i] = t0;
          B[jj+1][ii+i] = t1;
          B[jj+2][ii+i] = t2;
          B[jj+3][ii+i] = t3;
          B[jj+4][ii+i] = t4;
          B[jj+5][ii+i] = t5;
          B[jj+6][ii+i] = t6;
          B[jj+7][ii+i] = t7;
        }
      }
    }
  }

  /* Transpose 64x64 matrix:
   *
   * 8x8 submatrices doesn't work well, and 4x4 ones is not good enough.
   * So we should do more work to deal with conflict misses:
   * Divide matrix into 64(8x8) 8x8 submatrices, and in each 8x8 submatrices,
   * divide it into 4x4 ones.
   * Then transpose each 8x8 submatrices Ai to Bi, for i from 0 to 63.
   *
   * Ai     ->  Bi
   *
   * a0 a1  ->  b0=a0' b1=a2'
   * a2 a3  ->  b2=a1' b3=a3'
   *
   * Phase 1: a0 trans to b0, a1 trans to b1.
   * for i from 0 to 3:
   *     Step 1: cache the ist row a0 & a1 to 8 local variables.
   *     Step 2: move the first 4 variables to the ist column of b0.
   *     Step 3: move the last 4 variables to the ist column of b1.
   * Phase 2: b1 move to b2, a2 trans to b1.
   * for i from 0 to 3:
   *     Step 1: cache the ist row of b1 to 4 local variables.
   *     Step 2: trans the ist column of a2 to the ist row of b1.
   *     Step 3: move the 4 local variables to the ist b2.
   * Phase 3: a3 trans b3.
   */
  else if (N == 64 && M == 64) {
    for (ii = 0; ii < N; ii += 8) {
      for (jj = 0; jj < M; jj += 8) {
        // Phase 1
        for (i = 0; i < 4; i++) {
          t0 = A[ii+i][jj];
          t1 = A[ii+i][jj+1];
          t2 = A[ii+i][jj+2];
          t3 = A[ii+i][jj+3];
          t4 = A[ii+i][jj+4];
          t5 = A[ii+i][jj+5];
          t6 = A[ii+i][jj+6];
          t7 = A[ii+i][jj+7];
          
          B[jj][ii+i] = t0;
          B[jj+1][ii+i] = t1;
          B[jj+2][ii+i] = t2;
          B[jj+3][ii+i] = t3;

          B[jj][ii+i+4] = t4;
          B[jj+1][ii+i+4] = t5;
          B[jj+2][ii+i+4] = t6;
          B[jj+3][ii+i+4] = t7;
        }
        // Phase 2
        for (i = 0; i < 4; i++) {
          t0 = B[jj+i][ii+4];
          t1 = B[jj+i][ii+5];
          t2 = B[jj+i][ii+6];
          t3 = B[jj+i][ii+7];

          t4 = A[ii+4][jj+i];
          t5 = A[ii+5][jj+i];
          t6 = A[ii+6][jj+i];
          t7 = A[ii+7][jj+i];

          B[jj+i][ii+4] = t4;
          B[jj+i][ii+5] = t5;
          B[jj+i][ii+6] = t6;
          B[jj+i][ii+7] = t7;

          B[jj+4+i][ii] = t0;
          B[jj+4+i][ii+1] = t1;
          B[jj+4+i][ii+2] = t2;
          B[jj+4+i][ii+3] = t3;          
        }
        // Phase 3
        for (i = 4; i < 8; i++) {
          t4 = A[ii+i][jj+4];
          t5 = A[ii+i][jj+5];
          t6 = A[ii+i][jj+6];
          t7 = A[ii+i][jj+7];

          B[jj+4][ii+i] = t4;
          B[jj+5][ii+i] = t5;
          B[jj+6][ii+i] = t6;
          B[jj+7][ii+i] = t7;
        }
      }
    }
  }

  /* Transpose 61x67 matrix */
  else if (N == 67 && M == 61) {
    for (ii = 0; ii < N; ii += 18) {
      for (jj = 0; jj < M; jj += 18) {
        for (i = ii; i < ii + 18 && i < N; i++) {
          for (j = jj; j < jj + 18 && j < M; j++) {
            B[j][i] = A[i][j];
          }
        }
      }
    }
  }

}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

