#include "hclib.h"
/**
 *
 * @file pdgeqrf.c
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
#if defined(USE_OMPEXT)
#include <omp_ext.h>
#endif

#define A(m,n) BLKADDR(A, double, m, n)
#define T(m,n) BLKADDR(T, double, m, n)

/***************************************************************************//**
 *  Parallel tile QR factorization - dynamic scheduling
 **/
typedef struct _pragma46 {
    double *dA;
    double *dT;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempkn;
    int tempnn;
    int tempmm;
    PLASMA_desc A;
    PLASMA_desc T;
    int ib;
 } pragma46;

typedef struct _pragma58 {
    double *dA;
    double *dT;
    double *dC;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempkn;
    int tempnn;
    int tempmm;
    PLASMA_desc A;
    PLASMA_desc T;
    int ib;
 } pragma58;

typedef struct _pragma75 {
    double *dA;
    double *dB;
    double *dT;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempkn;
    int tempnn;
    int tempmm;
    PLASMA_desc A;
    PLASMA_desc T;
    int ib;
 } pragma75;

typedef struct _pragma91 {
    double *dA;
    double *dB;
    double *dV;
    double *dT;
    int k;
    int m;
    int n;
    int ldak;
    int ldam;
    int tempkm;
    int tempkn;
    int tempnn;
    int tempmm;
    PLASMA_desc A;
    PLASMA_desc T;
    int ib;
 } pragma91;

static void *pragma46_hclib_async(void *____arg);
static void *pragma58_hclib_async(void *____arg);
static void *pragma75_hclib_async(void *____arg);
static void *pragma91_hclib_async(void *____arg);
void plasma_pdgeqrf_quark(PLASMA_desc A, PLASMA_desc T, int ib)
{

    int k, m, n;
    int ldak, ldam;
    int tempkm, tempkn, tempnn, tempmm;

    for (k = 0; k < min(A.mt, A.nt); k++) {
        tempkm = k == A.mt-1 ? A.m-k*A.mb : A.mb;
        tempkn = k == A.nt-1 ? A.n-k*A.nb : A.nb;
        ldak = BLKLDD(A, k);
        double *dA = A(k, k);
        double *dT = T(k, k);
#if defined(USE_OMPEXT)
omp_set_task_priority(1);
#endif
 { 
pragma46 *ctx = (pragma46 *)malloc(sizeof(pragma46));
ctx->dA = dA;
ctx->dT = dT;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempnn = tempnn;
ctx->tempmm = tempmm;
ctx->A = A;
ctx->T = T;
ctx->ib = ib;
hclib_emulate_omp_task(pragma46_hclib_async, ctx, ANY_PLACE, 1, 2, (dA) + (0), T.nb*T.nb, (dA) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb);
 } 

        for (n = k+1; n < A.nt; n++) {
            tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
            double *dA = A(k, k);
            double *dT = T(k, k);
            double *dC = A(k, n);
 { 
pragma58 *ctx = (pragma58 *)malloc(sizeof(pragma58));
ctx->dA = dA;
ctx->dT = dT;
ctx->dC = dC;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempnn = tempnn;
ctx->tempmm = tempmm;
ctx->A = A;
ctx->T = T;
ctx->ib = ib;
hclib_emulate_omp_task(pragma58_hclib_async, ctx, ANY_PLACE, 3, 1, (dA) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb, (dC) + (0), T.nb*T.nb, (dC) + (0), T.nb*T.nb);
 } 
        }
        for (m = k+1; m < A.mt; m++) {
            tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
            ldam = BLKLDD(A, m);
            double *dA = A(k, k);
            double *dB = A(m, k);
            double *dT = T(m, k);
 { 
pragma75 *ctx = (pragma75 *)malloc(sizeof(pragma75));
ctx->dA = dA;
ctx->dB = dB;
ctx->dT = dT;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempnn = tempnn;
ctx->tempmm = tempmm;
ctx->A = A;
ctx->T = T;
ctx->ib = ib;
hclib_emulate_omp_task(pragma75_hclib_async, ctx, ANY_PLACE, 2, 3, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb);
 } 

            for (n = k+1; n < A.nt; n++) {
                tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
                double *dA = A(k, n);
                double *dB = A(m, n);
                double *dV = A(m, k);
                double *dT = T(m, k);
 { 
pragma91 *ctx = (pragma91 *)malloc(sizeof(pragma91));
ctx->dA = dA;
ctx->dB = dB;
ctx->dV = dV;
ctx->dT = dT;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempnn = tempnn;
ctx->tempmm = tempmm;
ctx->A = A;
ctx->T = T;
ctx->ib = ib;
hclib_emulate_omp_task(pragma91_hclib_async, ctx, ANY_PLACE, 4, 2, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb, (dV) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb);
 } 
            }
        }
    }
} 
static void *pragma46_hclib_async(void *____arg) {
    pragma46 *ctx = (pragma46 *)____arg;
    double *dA; dA = ctx->dA;
    double *dT; dT = ctx->dT;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempnn; tempnn = ctx->tempnn;
    int tempmm; tempmm = ctx->tempmm;
    PLASMA_desc A; A = ctx->A;
    PLASMA_desc T; T = ctx->T;
    int ib; ib = ctx->ib;
    hclib_start_finish();
{
            double tau[T.nb];
            double work[ib * T.nb];
            CORE_dgeqrt(tempkm, tempkn, ib, dA, ldak, dT, T.mb, &tau[0], &work[0]);
        } ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma58_hclib_async(void *____arg) {
    pragma58 *ctx = (pragma58 *)____arg;
    double *dA; dA = ctx->dA;
    double *dT; dT = ctx->dT;
    double *dC; dC = ctx->dC;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempnn; tempnn = ctx->tempnn;
    int tempmm; tempmm = ctx->tempmm;
    PLASMA_desc A; A = ctx->A;
    PLASMA_desc T; T = ctx->T;
    int ib; ib = ctx->ib;
    hclib_start_finish();
{
                double work[T.nb * ib];
                CORE_dormqr(PlasmaLeft, PlasmaTrans,
                        tempkm, tempnn, tempkm, ib,
                        dA, ldak,
                        dT, T.mb,
                        dC, ldak,
                        &work[0], T.nb);
            } ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma75_hclib_async(void *____arg) {
    pragma75 *ctx = (pragma75 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    double *dT; dT = ctx->dT;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempnn; tempnn = ctx->tempnn;
    int tempmm; tempmm = ctx->tempmm;
    PLASMA_desc A; A = ctx->A;
    PLASMA_desc T; T = ctx->T;
    int ib; ib = ctx->ib;
    hclib_start_finish();
{
                double tau[T.nb];
                double work[ib * T.nb];
                CORE_dtsqrt(tempmm, tempkn, ib,
                        dA, ldak,
                        dB, ldam,
                        dT, T.mb, &tau[0], &work[0]);
            } ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma91_hclib_async(void *____arg) {
    pragma91 *ctx = (pragma91 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    double *dV; dV = ctx->dV;
    double *dT; dT = ctx->dT;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempnn; tempnn = ctx->tempnn;
    int tempmm; tempmm = ctx->tempmm;
    PLASMA_desc A; A = ctx->A;
    PLASMA_desc T; T = ctx->T;
    int ib; ib = ctx->ib;
    hclib_start_finish();
{
                    double work[ib * T.nb];
                    CORE_dtsmqr(PlasmaLeft, PlasmaTrans,
                            A.mb, tempnn, tempmm, tempnn, A.nb, ib,
                            dA, ldak,
                            dB, ldam,
                            dV, ldam,
                            dT, T.mb, &work[0], ib);
                } ;     ; hclib_end_finish();
    return NULL;
}


