#include "hclib.h"
/**
 *
 * @file pdplgsy.c
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
#if defined(USE_OMPEXT)
#include <omp_ext.h>
#endif

#define A(m,n) BLKADDR(A, double, m, n)

/***************************************************************************//**
 *  Parallel tile Cholesky factorization - dynamic scheduling
 **/
typedef struct _pragma43 {
    double *dA;
    int m;
    int n;
    int ldam;
    int tempmm;
    int tempnn;
    double bump;
    PLASMA_desc A;
    unsigned long long seed;
 } pragma43;

static void *pragma43_hclib_async(void *____arg);
void plasma_pdplgsy_quark( double bump, PLASMA_desc A, unsigned long long int seed)
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
#if defined(USE_OMPEXT)
omp_set_task_affinity( (n%4)*6+(m%6) );
#endif
 { 
pragma43 *ctx = (pragma43 *)malloc(sizeof(pragma43));
ctx->dA = dA;
ctx->m = m;
ctx->n = n;
ctx->ldam = ldam;
ctx->tempmm = tempmm;
ctx->tempnn = tempnn;
ctx->bump = bump;
ctx->A = A;
ctx->seed = seed;
hclib_emulate_omp_task(pragma43_hclib_async, ctx, ANY_PLACE, 0, 1, (dA) + (0), ldam*tempnn);
 } ;
        }
    }
} 
static void *pragma43_hclib_async(void *____arg) {
    pragma43 *ctx = (pragma43 *)____arg;
    double *dA; dA = ctx->dA;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldam; ldam = ctx->ldam;
    int tempmm; tempmm = ctx->tempmm;
    int tempnn; tempnn = ctx->tempnn;
    double bump; bump = ctx->bump;
    PLASMA_desc A; A = ctx->A;
    unsigned long long seed; seed = ctx->seed;
    hclib_start_finish();
CORE_dplgsy( bump, tempmm, tempnn, dA, ldam, A.m, m*A.mb, n*A.nb, seed ) ;     ; hclib_end_finish();
    return NULL;
}


