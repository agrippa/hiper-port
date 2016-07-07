#include "hclib.h"
#ifdef __cplusplus
#include "hclib_cpp.h"
#include "hclib_system.h"
#ifdef __CUDACC__
#include "hclib_cuda.h"
#endif
#endif
/**
 *
 * @file pdpltmg.c
 *
 *  PLASMA auxiliary routines
 *  PLASMA is a software package provided by Univ. of Tennessee,
 *  Univ. of California Berkeley and Univ. of Colorado Denver
 *
 * @version 2.6.0
 * @author Mathieu Faverge
 * @date 2010-11-15
 * @generated d Tue Jan  7 11:45:12 2014
 *
 **/
#include "common.h"

#define A(m, n) BLKADDR(A, double, m, n)

/***************************************************************************//**
 *  Parallel tile matrix generation - dynamic scheduling
 **/
typedef struct _pragma45_omp_task {
    double (*(*dA_ptr));
    int (*m_ptr);
    int (*n_ptr);
    int (*ldam_ptr);
    int (*tempmm_ptr);
    int (*tempnn_ptr);
    PLASMA_desc (*A_ptr);
    unsigned long long (*seed_ptr);
 } pragma45_omp_task;

static void *pragma45_omp_task_hclib_async(void *____arg);
void plasma_pdpltmg_quark(PLASMA_desc A, unsigned long long int seed)
{
    int m, n;
    int ldam;
    int tempmm, tempnn;

    for (m = 0; m < A.mt; m++) {
        tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
        ldam = BLKLDD(A, m);

        for (n = 0; n < A.nt; n++) {
            tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
            double *dA = A(m, n);
 { 
pragma45_omp_task *new_ctx = (pragma45_omp_task *)malloc(sizeof(pragma45_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->ldam_ptr = &(ldam);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->A_ptr = &(A);
new_ctx->seed_ptr = &(seed);
hclib_emulate_omp_task(pragma45_omp_task_hclib_async, new_ctx, ANY_PLACE, 0, 1, (dA) + (0), tempnn*ldam);
 } ;
        }
    }
} 
static void *pragma45_omp_task_hclib_async(void *____arg) {
    pragma45_omp_task *ctx = (pragma45_omp_task *)____arg;
    hclib_start_finish();
CORE_dplrnt((*(ctx->tempmm_ptr)), (*(ctx->tempnn_ptr)), (*(ctx->dA_ptr)), (*(ctx->ldam_ptr)), (*(ctx->A_ptr)).m, (*(ctx->m_ptr))*(*(ctx->A_ptr)).mb, (*(ctx->n_ptr))*(*(ctx->A_ptr)).nb, (*(ctx->seed_ptr))) ;     ; hclib_end_finish_nonblocking();

    free(____arg);
    return NULL;
}



/***************************************************************************//**
 *  Parallel tile matrix generation - dynamic scheduling
 *  Same as previous function, without OpenMP pragma. Used to check solution.
 **/
void plasma_pdpltmg_seq(PLASMA_desc A, unsigned long long int seed)
{
    int m, n;
    int ldam;
    int tempmm, tempnn;

    for (m = 0; m < A.mt; m++) {
        tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
        ldam = BLKLDD(A, m);

        for (n = 0; n < A.nt; n++) {
            tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
            double *dA = A(m, n);
            CORE_dplrnt(tempmm, tempnn, dA, ldam, A.m, m*A.mb, n*A.nb, seed);
        }
    }
}

