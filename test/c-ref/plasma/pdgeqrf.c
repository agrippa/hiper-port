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
typedef struct _pragma54_omp_task {
    double (*(*dA_ptr));
    double (*(*dT_ptr));
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempnn_ptr);
    int (*tempmm_ptr);
    PLASMA_desc (*A_ptr);
    PLASMA_desc (*T_ptr);
    int (*ib_ptr);
 } pragma54_omp_task;

typedef struct _pragma66_omp_task {
    double (*(*dA_ptr));
    double (*(*dT_ptr));
    double (*(*dC_ptr));
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempnn_ptr);
    int (*tempmm_ptr);
    PLASMA_desc (*A_ptr);
    PLASMA_desc (*T_ptr);
    int (*ib_ptr);
 } pragma66_omp_task;

typedef struct _pragma83_omp_task {
    double (*(*dA_ptr));
    double (*(*dB_ptr));
    double (*(*dT_ptr));
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempnn_ptr);
    int (*tempmm_ptr);
    PLASMA_desc (*A_ptr);
    PLASMA_desc (*T_ptr);
    int (*ib_ptr);
 } pragma83_omp_task;

typedef struct _pragma99_omp_task {
    double (*(*dA_ptr));
    double (*(*dB_ptr));
    double (*(*dV_ptr));
    double (*(*dT_ptr));
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempnn_ptr);
    int (*tempmm_ptr);
    PLASMA_desc (*A_ptr);
    PLASMA_desc (*T_ptr);
    int (*ib_ptr);
 } pragma99_omp_task;

static void *pragma54_omp_task_hclib_async(void *____arg);
static void *pragma66_omp_task_hclib_async(void *____arg);
static void *pragma83_omp_task_hclib_async(void *____arg);
static void *pragma99_omp_task_hclib_async(void *____arg);
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
pragma54_omp_task *new_ctx = (pragma54_omp_task *)malloc(sizeof(pragma54_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dT_ptr = &(dT);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->A_ptr = &(A);
new_ctx->T_ptr = &(T);
new_ctx->ib_ptr = &(ib);
hclib_emulate_omp_task(pragma54_omp_task_hclib_async, new_ctx, ANY_PLACE, 1, 2, (dA) + (0), T.nb*T.nb, (dA) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb);
 } 

        for (n = k+1; n < A.nt; n++) {
            tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
            double *dA = A(k, k);
            double *dT = T(k, k);
            double *dC = A(k, n);
 { 
pragma66_omp_task *new_ctx = (pragma66_omp_task *)malloc(sizeof(pragma66_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dT_ptr = &(dT);
new_ctx->dC_ptr = &(dC);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->A_ptr = &(A);
new_ctx->T_ptr = &(T);
new_ctx->ib_ptr = &(ib);
hclib_emulate_omp_task(pragma66_omp_task_hclib_async, new_ctx, ANY_PLACE, 3, 1, (dA) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb, (dC) + (0), T.nb*T.nb, (dC) + (0), T.nb*T.nb);
 } 
        }
        for (m = k+1; m < A.mt; m++) {
            tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
            ldam = BLKLDD(A, m);
            double *dA = A(k, k);
            double *dB = A(m, k);
            double *dT = T(m, k);
 { 
pragma83_omp_task *new_ctx = (pragma83_omp_task *)malloc(sizeof(pragma83_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dB_ptr = &(dB);
new_ctx->dT_ptr = &(dT);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->A_ptr = &(A);
new_ctx->T_ptr = &(T);
new_ctx->ib_ptr = &(ib);
hclib_emulate_omp_task(pragma83_omp_task_hclib_async, new_ctx, ANY_PLACE, 2, 3, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb);
 } 

            for (n = k+1; n < A.nt; n++) {
                tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
                double *dA = A(k, n);
                double *dB = A(m, n);
                double *dV = A(m, k);
                double *dT = T(m, k);
 { 
pragma99_omp_task *new_ctx = (pragma99_omp_task *)malloc(sizeof(pragma99_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dB_ptr = &(dB);
new_ctx->dV_ptr = &(dV);
new_ctx->dT_ptr = &(dT);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->A_ptr = &(A);
new_ctx->T_ptr = &(T);
new_ctx->ib_ptr = &(ib);
hclib_emulate_omp_task(pragma99_omp_task_hclib_async, new_ctx, ANY_PLACE, 4, 2, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb, (dV) + (0), T.nb*T.nb, (dT) + (0), ib*T.nb, (dA) + (0), T.nb*T.nb, (dB) + (0), T.nb*T.nb);
 } 
            }
        }
    }
} 

#ifdef OMP_TO_HCLIB_ENABLE_GPU

static void *pragma54_omp_task_hclib_async(void *____arg) {
    pragma54_omp_task *ctx = (pragma54_omp_task *)____arg;
    hclib_start_finish();
{
            double tau[(*(ctx->T_ptr)).nb];
            double work[(*(ctx->ib_ptr)) * (*(ctx->T_ptr)).nb];
            CORE_dgeqrt((*(ctx->tempkm_ptr)), (*(ctx->tempkn_ptr)), (*(ctx->ib_ptr)), (*(ctx->dA_ptr)), (*(ctx->ldak_ptr)), (*(ctx->dT_ptr)), (*(ctx->T_ptr)).mb, &tau[0], &work[0]);
        } ;     ; hclib_end_finish_nonblocking();

    free(____arg);
    return NULL;
}

#endif


#ifdef OMP_TO_HCLIB_ENABLE_GPU

static void *pragma66_omp_task_hclib_async(void *____arg) {
    pragma66_omp_task *ctx = (pragma66_omp_task *)____arg;
    hclib_start_finish();
{
                double work[(*(ctx->T_ptr)).nb * (*(ctx->ib_ptr))];
                CORE_dormqr(PlasmaLeft, PlasmaTrans,
                        (*(ctx->tempkm_ptr)), (*(ctx->tempnn_ptr)), (*(ctx->tempkm_ptr)), (*(ctx->ib_ptr)),
                        (*(ctx->dA_ptr)), (*(ctx->ldak_ptr)),
                        (*(ctx->dT_ptr)), (*(ctx->T_ptr)).mb,
                        (*(ctx->dC_ptr)), (*(ctx->ldak_ptr)),
                        &work[0], (*(ctx->T_ptr)).nb);
            } ;     ; hclib_end_finish_nonblocking();

    free(____arg);
    return NULL;
}

#endif


#ifdef OMP_TO_HCLIB_ENABLE_GPU

static void *pragma83_omp_task_hclib_async(void *____arg) {
    pragma83_omp_task *ctx = (pragma83_omp_task *)____arg;
    hclib_start_finish();
{
                double tau[(*(ctx->T_ptr)).nb];
                double work[(*(ctx->ib_ptr)) * (*(ctx->T_ptr)).nb];
                CORE_dtsqrt((*(ctx->tempmm_ptr)), (*(ctx->tempkn_ptr)), (*(ctx->ib_ptr)),
                        (*(ctx->dA_ptr)), (*(ctx->ldak_ptr)),
                        (*(ctx->dB_ptr)), (*(ctx->ldam_ptr)),
                        (*(ctx->dT_ptr)), (*(ctx->T_ptr)).mb, &tau[0], &work[0]);
            } ;     ; hclib_end_finish_nonblocking();

    free(____arg);
    return NULL;
}

#endif


#ifdef OMP_TO_HCLIB_ENABLE_GPU

static void *pragma99_omp_task_hclib_async(void *____arg) {
    pragma99_omp_task *ctx = (pragma99_omp_task *)____arg;
    hclib_start_finish();
{
                    double work[(*(ctx->ib_ptr)) * (*(ctx->T_ptr)).nb];
                    CORE_dtsmqr(PlasmaLeft, PlasmaTrans,
                            (*(ctx->A_ptr)).mb, (*(ctx->tempnn_ptr)), (*(ctx->tempmm_ptr)), (*(ctx->tempnn_ptr)), (*(ctx->A_ptr)).nb, (*(ctx->ib_ptr)),
                            (*(ctx->dA_ptr)), (*(ctx->ldak_ptr)),
                            (*(ctx->dB_ptr)), (*(ctx->ldam_ptr)),
                            (*(ctx->dV_ptr)), (*(ctx->ldam_ptr)),
                            (*(ctx->dT_ptr)), (*(ctx->T_ptr)).mb, &work[0], (*(ctx->ib_ptr)));
                } ;     ; hclib_end_finish_nonblocking();

    free(____arg);
    return NULL;
}

#endif

