#include "hclib.h"
/**
 *
 * @file pdpotrf.c
 *
 *  PLASMA auxiliary routines
 *  PLASMA is a software package provided by Univ. of Tennessee,
 *  Univ. of California Berkeley and Univ. of Colorado Denver
 *
 * @version 2.6.0
 * @author Jakub Kurzak
 * @author Hatem Ltaief
 * @author Mathieu Faverge
 * @date 2010-11-15
 * @generated d Tue Jan  7 11:45:10 2014
 *
 **/
#include "common.h"
#ifdef USE_MKL
#include <mkl_lapacke.h>
#else
#include <lapacke.h>
#endif
#if defined(USE_OMPEXT)
#include <omp_ext.h>
#endif

#define A(m,n) BLKADDR(A, double, m, n)

/***************************************************************************//**
 *  Parallel tile Cholesky factorization - dynamic scheduling
 **/
typedef struct _pragma59 {
    double *dA;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempmm;
    double zone;
    double mzone;
    PLASMA_enum uplo;
    PLASMA_desc A;
 } pragma59;

typedef struct _pragma68 {
    double *dA;
    double *dB;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempmm;
    double zone;
    double mzone;
    PLASMA_enum uplo;
    PLASMA_desc A;
 } pragma68;

typedef struct _pragma82 {
    double *dA;
    double *dB;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempmm;
    double zone;
    double mzone;
    PLASMA_enum uplo;
    PLASMA_desc A;
 } pragma82;

typedef struct _pragma96 {
    double *dA;
    double *dB;
    double *dC;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempmm;
    double zone;
    double mzone;
    PLASMA_enum uplo;
    PLASMA_desc A;
 } pragma96;

static void *pragma59_hclib_async(void *____arg);
static void *pragma68_hclib_async(void *____arg);
static void *pragma82_hclib_async(void *____arg);
static void *pragma96_hclib_async(void *____arg);
void plasma_pdpotrf_quark(PLASMA_enum uplo, PLASMA_desc A)
{
    int k, m, n;
    int ldak, ldam;
    int tempkm, tempmm;

    double zone  = (double) 1.0;
    double mzone = (double)-1.0;
    /*
     *  PlasmaLower
     */
    if (uplo == PlasmaLower) {
        abort();
    }
    /*
     *  PlasmaUpper
     */
    else {
        for (k = 0; k < A.nt; k++) {
            tempkm = k == A.nt-1 ? A.n-k*A.nb : A.nb;
            ldak = BLKLDD(A, k);
            double *dA = A(k, k);
#if defined(USE_OMPEXT)
omp_set_task_priority(1);
#endif
 { 
pragma59 *ctx = (pragma59 *)malloc(sizeof(pragma59));
ctx->dA = dA;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempmm = tempmm;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->uplo = uplo;
ctx->A = A;
hclib_emulate_omp_task(pragma59_hclib_async, ctx, ANY_PLACE, 1, 1, (dA) + (0), A.mb*A.mb, (dA) + (0), A.mb*A.mb);
 } 

            for (m = k+1; m < A.nt; m++) {
                tempmm = m == A.nt-1 ? A.n-m*A.nb : A.nb;
                double *dA = A(k, k);
                double *dB = A(k, m);
 { 
pragma68 *ctx = (pragma68 *)malloc(sizeof(pragma68));
ctx->dA = dA;
ctx->dB = dB;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempmm = tempmm;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->uplo = uplo;
ctx->A = A;
hclib_emulate_omp_task(pragma68_hclib_async, ctx, ANY_PLACE, 2, 1, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb);
 } ;
            }
            for (m = k+1; m < A.nt; m++) {
                tempmm = m == A.nt-1 ? A.n-m*A.nb : A.nb;
                ldam = BLKLDD(A, m);
                double *dA = A(k, m);
                double *dB = A(m, m);
 { 
pragma82 *ctx = (pragma82 *)malloc(sizeof(pragma82));
ctx->dA = dA;
ctx->dB = dB;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempmm = tempmm;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->uplo = uplo;
ctx->A = A;
hclib_emulate_omp_task(pragma82_hclib_async, ctx, ANY_PLACE, 2, 1, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb);
 } 

                for (n = k+1; n < m; n++) {
                    double *dA = A(k , n);
                    double *dB = A(k , m);
                    double *dC = A(n , m);
 { 
pragma96 *ctx = (pragma96 *)malloc(sizeof(pragma96));
ctx->dA = dA;
ctx->dB = dB;
ctx->dC = dC;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempmm = tempmm;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->uplo = uplo;
ctx->A = A;
hclib_emulate_omp_task(pragma96_hclib_async, ctx, ANY_PLACE, 3, 1, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (dC) + (0), A.mb*A.mb, (dC) + (0), A.mb*A.mb);
 } ;
                }
            }
        }
    }
} 
static void *pragma59_hclib_async(void *____arg) {
    pragma59 *ctx = (pragma59 *)____arg;
    double *dA; dA = ctx->dA;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempmm; tempmm = ctx->tempmm;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    PLASMA_enum uplo; uplo = ctx->uplo;
    PLASMA_desc A; A = ctx->A;
    hclib_start_finish();
{
                LAPACKE_dpotrf_work(LAPACK_COL_MAJOR, lapack_const(PlasmaUpper), tempkm, dA, ldak);
            } ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma68_hclib_async(void *____arg) {
    pragma68 *ctx = (pragma68 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempmm; tempmm = ctx->tempmm;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    PLASMA_enum uplo; uplo = ctx->uplo;
    PLASMA_desc A; A = ctx->A;
    hclib_start_finish();
cblas_dtrsm(
                        CblasColMajor,
                        (CBLAS_SIDE)PlasmaLeft, (CBLAS_UPLO)PlasmaUpper,
                        (CBLAS_TRANSPOSE)PlasmaTrans, (CBLAS_DIAG)PlasmaNonUnit,
                        A.mb, tempmm,
                        zone, dA, ldak,
                        dB, ldak) ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma82_hclib_async(void *____arg) {
    pragma82 *ctx = (pragma82 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempmm; tempmm = ctx->tempmm;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    PLASMA_enum uplo; uplo = ctx->uplo;
    PLASMA_desc A; A = ctx->A;
    hclib_start_finish();
{
                    cblas_dsyrk(
                            CblasColMajor,
                            (CBLAS_UPLO)PlasmaUpper, (CBLAS_TRANSPOSE)PlasmaTrans,
                            tempmm, A.mb,
                            (-1.0), dA, ldak,
                            (1.0), dB, ldam);
                } ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma96_hclib_async(void *____arg) {
    pragma96 *ctx = (pragma96 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    double *dC; dC = ctx->dC;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempmm; tempmm = ctx->tempmm;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    PLASMA_enum uplo; uplo = ctx->uplo;
    PLASMA_desc A; A = ctx->A;
    hclib_start_finish();
cblas_dgemm(CblasColMajor, (CBLAS_TRANSPOSE)PlasmaTrans, (CBLAS_TRANSPOSE)PlasmaNoTrans,
                            A.mb, tempmm, A.mb,
                            mzone, dA, ldak,
                            dB, ldak,
                            zone, dC, A.mb) ;     ; hclib_end_finish();
    return NULL;
}


