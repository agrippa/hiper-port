#include "hclib.h"
#ifdef __cplusplus
#include "hclib_cpp.h"
#include "hclib_system.h"
#include "hclib_openshmem.h"
#endif
/**
 *
 * @file pdgetrf_rectil.c
 *
 *  PLASMA auxiliary routines
 *  PLASMA is a software package provided by Univ. of Tennessee,
 *  Univ. of California Berkeley and Univ. of Colorado Denver
 *
 * LU with Partial pivoting.
 *
 * @version 2.6.0
 * @author Mathieu Faverge
 * @author Hatem Ltaief
 * @date 2009-11-15
 *
 * @generated d Tue Jan  7 11:45:13 2014
 *
 **/
#include "common.h"

void CORE_dgetrf_rectil_init(void);

#define PARALLEL_KERNEL
#define A(m,n) BLKADDR(A, double, m, n)
#define IPIV(k) &(IPIV[(int64_t)A.mb*(int64_t)(k)])


/***************************************************************************//**
 *  Parallel tile LU factorization - dynamic scheduling - Right looking
 **/
typedef struct _pragma63_omp_task {
    double (*(*dA_ptr));
    int (*(*dB_ptr));
    PLASMA_desc (*pDesc_ptr);
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*tempk_ptr);
    int (*tempm_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempmm_ptr);
    int (*tempnn_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    double (*zone_ptr);
    double (*mzone_ptr);
    void (*(*fakedep_ptr));
    PLASMA_desc (*A_ptr);
    int (*(*IPIV_ptr));
 } pragma63_omp_task;

typedef struct _pragma86_omp_task {
    PLASMA_desc (*descA_ptr);
    double (*(*dA_ptr));
    double (*(*dB_ptr));
    int (*(*dipiv_ptr));
    PLASMA_desc (*pDesc_ptr);
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*tempk_ptr);
    int (*tempm_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempmm_ptr);
    int (*tempnn_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    double (*zone_ptr);
    double (*mzone_ptr);
    void (*(*fakedep_ptr));
    PLASMA_desc (*A_ptr);
    int (*(*IPIV_ptr));
 } pragma86_omp_task;

typedef struct _pragma97_omp_task {
    double (*(*dA_ptr));
    double (*(*dB_ptr));
    double (*(*dC_ptr));
    PLASMA_desc (*descA_ptr);
    int (*(*dipiv_ptr));
    PLASMA_desc (*pDesc_ptr);
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*tempk_ptr);
    int (*tempm_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempmm_ptr);
    int (*tempnn_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    double (*zone_ptr);
    double (*mzone_ptr);
    void (*(*fakedep_ptr));
    PLASMA_desc (*A_ptr);
    int (*(*IPIV_ptr));
 } pragma97_omp_task;

typedef struct _pragma123_omp_task {
    double (*(*dA_ptr));
    double (*(*dB_ptr));
    double (*(*dC_ptr));
    double (*(*fake1_ptr));
    double (*(*fake2_ptr));
    PLASMA_desc (*descA_ptr);
    int (*(*dipiv_ptr));
    PLASMA_desc (*pDesc_ptr);
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*tempk_ptr);
    int (*tempm_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempmm_ptr);
    int (*tempnn_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    double (*zone_ptr);
    double (*mzone_ptr);
    void (*(*fakedep_ptr));
    PLASMA_desc (*A_ptr);
    int (*(*IPIV_ptr));
 } pragma123_omp_task;

typedef struct _pragma161_omp_task {
    double (*(*Aij_ptr));
    double (*(*prevSwap_ptr));
    int (*(*dipiv_ptr));
    PLASMA_desc (*descA_ptr);
    int (*mintmp_ptr);
    int (*k_ptr);
    int (*m_ptr);
    int (*n_ptr);
    int (*tempk_ptr);
    int (*tempm_ptr);
    int (*tempkm_ptr);
    int (*tempkn_ptr);
    int (*tempmm_ptr);
    int (*tempnn_ptr);
    int (*ldak_ptr);
    int (*ldam_ptr);
    double (*zone_ptr);
    double (*mzone_ptr);
    void (*(*fakedep_ptr));
    PLASMA_desc (*A_ptr);
    int (*(*IPIV_ptr));
 } pragma161_omp_task;

static void *pragma63_omp_task_hclib_async(void *____arg);
static void *pragma86_omp_task_hclib_async(void *____arg);
static void *pragma97_omp_task_hclib_async(void *____arg);
static void *pragma123_omp_task_hclib_async(void *____arg);
static void *pragma161_omp_task_hclib_async(void *____arg);
void plasma_pdgetrf_rectil_quark(PLASMA_desc A, int *IPIV)
{
    int k, m, n;
    int tempk, tempm, tempkm, tempkn, tempmm, tempnn;
    int ldak, ldam;

    double zone  = (double)1.0;
    double mzone = (double)-1.0;

    void * fakedep;
    /* How many threads per panel? Probably needs to be adjusted during factorization. */

    CORE_dgetrf_rectil_init();

    for (k = 0; k < min(A.mt, A.nt); k++)
    {
        tempk  = k * A.mb;
        tempm  = A.m - tempk;
        tempkm = k == A.mt-1 ? tempm      : A.mb;
        tempkn = k == A.nt-1 ? A.n-k*A.nb : A.nb;
        ldak = BLKLDD(A, k);

        double *dA = A(k, k);
        int *dB = IPIV(k);
        PLASMA_desc pDesc = plasma_desc_submatrix(A, tempk, k*A.nb, tempm, tempkn);
 { 
pragma63_omp_task *new_ctx = (pragma63_omp_task *)malloc(sizeof(pragma63_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dB_ptr = &(dB);
new_ctx->pDesc_ptr = &(pDesc);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->tempk_ptr = &(tempk);
new_ctx->tempm_ptr = &(tempm);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->zone_ptr = &(zone);
new_ctx->mzone_ptr = &(mzone);
new_ctx->fakedep_ptr = &(fakedep);
new_ctx->A_ptr = &(A);
new_ctx->IPIV_ptr = &(IPIV);
hclib_emulate_omp_task(pragma63_omp_task_hclib_async, new_ctx, ANY_PLACE, 1, 2, (dA) + (0), A.mb*A.nb, (dA) + (0), A.mb*A.nb, (dB) + (0), pDesc.n);
 } 

        /*
         * Update the trailing submatrix
         */
        fakedep = (void *)(intptr_t)(k+1);
        for (n = k+1; n < A.nt; n++)
        {
            /*
             * Apply row interchange after the panel (work on the panel)
             */
            tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
            PLASMA_desc descA = plasma_desc_submatrix(A, tempk, n*A.nb, tempm, tempnn);
            double *dA = A(k, n);
            double *dB = A(k, k);
            int *dipiv = IPIV(k);
 { 
pragma86_omp_task *new_ctx = (pragma86_omp_task *)malloc(sizeof(pragma86_omp_task));
new_ctx->descA_ptr = &(descA);
new_ctx->dA_ptr = &(dA);
new_ctx->dB_ptr = &(dB);
new_ctx->dipiv_ptr = &(dipiv);
new_ctx->pDesc_ptr = &(pDesc);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->tempk_ptr = &(tempk);
new_ctx->tempm_ptr = &(tempm);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->zone_ptr = &(zone);
new_ctx->mzone_ptr = &(mzone);
new_ctx->fakedep_ptr = &(fakedep);
new_ctx->A_ptr = &(A);
new_ctx->IPIV_ptr = &(IPIV);
hclib_emulate_omp_task(pragma86_omp_task_hclib_async, new_ctx, ANY_PLACE, 3, 1, (dA) + (0), 1, (dB) + (0), ldak, (dipiv) + (0), tempkm, (dA) + (0), 1);
 } ;

            m = k+1;
            if ( m < A.mt ) {
                tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
                ldam = BLKLDD(A, m);

                double *dA = A(m , k);
                double *dB = A(k , n);
                double *dC = A(m , n);
 { 
pragma97_omp_task *new_ctx = (pragma97_omp_task *)malloc(sizeof(pragma97_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dB_ptr = &(dB);
new_ctx->dC_ptr = &(dC);
new_ctx->descA_ptr = &(descA);
new_ctx->dipiv_ptr = &(dipiv);
new_ctx->pDesc_ptr = &(pDesc);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->tempk_ptr = &(tempk);
new_ctx->tempm_ptr = &(tempm);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->zone_ptr = &(zone);
new_ctx->mzone_ptr = &(mzone);
new_ctx->fakedep_ptr = &(fakedep);
new_ctx->A_ptr = &(A);
new_ctx->IPIV_ptr = &(IPIV);
hclib_emulate_omp_task(pragma97_omp_task_hclib_async, new_ctx, ANY_PLACE, 3, 1, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (dC) + (0), A.mb*A.mb, (dC) + (0), A.mb*A.mb);
 } ;

                for (m = k+2; m < A.mt; m++)
                {
                    tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
                    ldam = BLKLDD(A, m);

                    double *dA = A(m , k);
                    double *dB = A(k , n);
                    double *dC = A(m , n);
                    double *fake1 = A(k+1, n);
                    double *fake2 = (double *)fakedep;
#if defined(KLANG_VERSION) && defined(KASTOR_USE_CW)
#warning "KLANG EXTENSION USED"
hclib_pragma_marker("omp", "task depend(in:dA[0:A.mb*A.mb], dB[0:A.mb*A.mb], fake2[0:1]) depend(inout:dC[0:A.mb*A.mb]), depend(cw:fake1[0:A.mb*A.nb])");
                        cblas_dgemm(CblasColMajor, (CBLAS_TRANSPOSE)PlasmaNoTrans, (CBLAS_TRANSPOSE)PlasmaNoTrans,
                                tempmm, tempnn, A.nb,
                                mzone, dA, ldam,
                                dB, ldak,
                                zone, dC, ldam);
#else
 { 
pragma123_omp_task *new_ctx = (pragma123_omp_task *)malloc(sizeof(pragma123_omp_task));
new_ctx->dA_ptr = &(dA);
new_ctx->dB_ptr = &(dB);
new_ctx->dC_ptr = &(dC);
new_ctx->fake1_ptr = &(fake1);
new_ctx->fake2_ptr = &(fake2);
new_ctx->descA_ptr = &(descA);
new_ctx->dipiv_ptr = &(dipiv);
new_ctx->pDesc_ptr = &(pDesc);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->tempk_ptr = &(tempk);
new_ctx->tempm_ptr = &(tempm);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->zone_ptr = &(zone);
new_ctx->mzone_ptr = &(mzone);
new_ctx->fakedep_ptr = &(fakedep);
new_ctx->A_ptr = &(A);
new_ctx->IPIV_ptr = &(IPIV);
hclib_emulate_omp_task(pragma123_omp_task_hclib_async, new_ctx, ANY_PLACE, 5, 2, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (fake2) + (0), 1, (dC) + (0), A.mb*A.mb, (fake1) + (0), A.mb*A.nb, (dC) + (0), A.mb*A.mb, (fake1) + (0), A.mb*A.nb);
 } ;
#endif
                }
            }
        }
    }

    for (k = 0; k < min(A.mt, A.nt); k++)
    {
        int mintmp;
        tempk  = k * A.mb;
        tempm  = A.m - tempk;
        tempkm = k == A.mt-1 ? tempm : A.mb;
        tempkn = k == A.nt-1 ? A.n - k * A.nb : A.nb;
        mintmp = min(tempkm, tempkn);
        ldak = BLKLDD(A, k);

        /*
         * Apply row interchange behind the panel (work on the panel)
         */
        fakedep = (void*)(intptr_t)k;
        for (n = 0; n < k; n++)
        {
            tempnn = n == A.nt-1 ? A.n-n*A.nb : A.nb;
            double *Aij = A(k, n);
            double *prevSwap = A(k-1, n);
            int *dipiv = IPIV(k);
            PLASMA_desc descA = plasma_desc_submatrix(A, tempk, n*A.nb, tempm, tempnn);
#if defined(KLANG_VERSION) && defined(KASTOR_USE_CW)
#warning "KLANG EXTENSION USED"
hclib_pragma_marker("omp", "task depend(inout:Aij[0:1]) depend(cw:fakedep) depend(in:dipiv[0:mintmp], prevSwap[0:A.lm*A.nb])");
            CORE_dlaswp_ontile(descA, 1, mintmp, dipiv, 1);
#else
 { 
pragma161_omp_task *new_ctx = (pragma161_omp_task *)malloc(sizeof(pragma161_omp_task));
new_ctx->Aij_ptr = &(Aij);
new_ctx->prevSwap_ptr = &(prevSwap);
new_ctx->dipiv_ptr = &(dipiv);
new_ctx->descA_ptr = &(descA);
new_ctx->mintmp_ptr = &(mintmp);
new_ctx->k_ptr = &(k);
new_ctx->m_ptr = &(m);
new_ctx->n_ptr = &(n);
new_ctx->tempk_ptr = &(tempk);
new_ctx->tempm_ptr = &(tempm);
new_ctx->tempkm_ptr = &(tempkm);
new_ctx->tempkn_ptr = &(tempkn);
new_ctx->tempmm_ptr = &(tempmm);
new_ctx->tempnn_ptr = &(tempnn);
new_ctx->ldak_ptr = &(ldak);
new_ctx->ldam_ptr = &(ldam);
new_ctx->zone_ptr = &(zone);
new_ctx->mzone_ptr = &(mzone);
new_ctx->fakedep_ptr = &(fakedep);
new_ctx->A_ptr = &(A);
new_ctx->IPIV_ptr = &(IPIV);
hclib_emulate_omp_task(pragma161_omp_task_hclib_async, new_ctx, ANY_PLACE, 4, 2, (Aij) + (0), 1, fakedep, 0, (dipiv) + (0), mintmp, (prevSwap) + (0), A.lm*A.nb, (Aij) + (0), 1, fakedep, 0);
 } ;
#endif
        }
    }
} 
static void *pragma63_omp_task_hclib_async(void *____arg) {
    pragma63_omp_task *ctx = (pragma63_omp_task *)____arg;
    hclib_start_finish();
{
            int info[3];
            info[1] = 0;
            info[2] = 1;

            CORE_dgetrf_rectil( (*(ctx->pDesc_ptr)), (*(ctx->dB_ptr)), info );
        } ;     ; hclib_end_finish();

    return NULL;
    free(____arg);
}


static void *pragma86_omp_task_hclib_async(void *____arg) {
    pragma86_omp_task *ctx = (pragma86_omp_task *)____arg;
    hclib_start_finish();
CORE_dswptr_ontile((*(ctx->descA_ptr)), 1, (*(ctx->tempkm_ptr)), (*(ctx->dipiv_ptr)), 1, (*(ctx->dB_ptr)), (*(ctx->ldak_ptr))) ;     ; hclib_end_finish();

    return NULL;
    free(____arg);
}


static void *pragma97_omp_task_hclib_async(void *____arg) {
    pragma97_omp_task *ctx = (pragma97_omp_task *)____arg;
    hclib_start_finish();
cblas_dgemm(CblasColMajor, (CBLAS_TRANSPOSE)PlasmaNoTrans, (CBLAS_TRANSPOSE)PlasmaNoTrans,
                        (*(ctx->tempmm_ptr)), (*(ctx->tempnn_ptr)), (*(ctx->A_ptr)).nb,
                        (*(ctx->mzone_ptr)), (*(ctx->dA_ptr)), (*(ctx->ldam_ptr)),
                        (*(ctx->dB_ptr)), (*(ctx->ldak_ptr)),
                        (*(ctx->zone_ptr)), (*(ctx->dC_ptr)), (*(ctx->ldam_ptr))) ;     ; hclib_end_finish();

    return NULL;
    free(____arg);
}


static void *pragma123_omp_task_hclib_async(void *____arg) {
    pragma123_omp_task *ctx = (pragma123_omp_task *)____arg;
    hclib_start_finish();
cblas_dgemm(CblasColMajor, (CBLAS_TRANSPOSE)PlasmaNoTrans, (CBLAS_TRANSPOSE)PlasmaNoTrans,
                                (*(ctx->tempmm_ptr)), (*(ctx->tempnn_ptr)), (*(ctx->A_ptr)).nb,
                                (*(ctx->mzone_ptr)), (*(ctx->dA_ptr)), (*(ctx->ldam_ptr)),
                                (*(ctx->dB_ptr)), (*(ctx->ldak_ptr)),
                                (*(ctx->zone_ptr)), (*(ctx->dC_ptr)), (*(ctx->ldam_ptr))) ;     ; hclib_end_finish();

    return NULL;
    free(____arg);
}


static void *pragma161_omp_task_hclib_async(void *____arg) {
    pragma161_omp_task *ctx = (pragma161_omp_task *)____arg;
    hclib_start_finish();
CORE_dlaswp_ontile((*(ctx->descA_ptr)), 1, (*(ctx->mintmp_ptr)), (*(ctx->dipiv_ptr)), 1) ;     ; hclib_end_finish();

    return NULL;
    free(____arg);
}


