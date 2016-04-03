#include "hclib.h"
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
typedef struct _pragma58 {
    double *dA;
    int *dB;
    PLASMA_desc pDesc;
    int k;
    int m;
    int n;
    int tempk;
    int tempm;
    int tempkm;
    int tempkn;
    int tempmm;
    int tempnn;
    int ldak;
    int ldam;
    double zone;
    double mzone;
    void *fakedep;
    PLASMA_desc A;
    int *IPIV;
 } pragma58;

typedef struct _pragma81 {
    PLASMA_desc descA;
    double *dA;
    double *dB;
    int *dipiv;
    PLASMA_desc pDesc;
    int k;
    int m;
    int n;
    int tempk;
    int tempm;
    int tempkm;
    int tempkn;
    int tempmm;
    int tempnn;
    int ldak;
    int ldam;
    double zone;
    double mzone;
    void *fakedep;
    PLASMA_desc A;
    int *IPIV;
 } pragma81;

typedef struct _pragma92 {
    double *dA;
    double *dB;
    double *dC;
    PLASMA_desc descA;
    int *dipiv;
    PLASMA_desc pDesc;
    int k;
    int m;
    int n;
    int tempk;
    int tempm;
    int tempkm;
    int tempkn;
    int tempmm;
    int tempnn;
    int ldak;
    int ldam;
    double zone;
    double mzone;
    void *fakedep;
    PLASMA_desc A;
    int *IPIV;
 } pragma92;

typedef struct _pragma118 {
    double *dA;
    double *dB;
    double *dC;
    double *fake1;
    double *fake2;
    PLASMA_desc descA;
    int *dipiv;
    PLASMA_desc pDesc;
    int k;
    int m;
    int n;
    int tempk;
    int tempm;
    int tempkm;
    int tempkn;
    int tempmm;
    int tempnn;
    int ldak;
    int ldam;
    double zone;
    double mzone;
    void *fakedep;
    PLASMA_desc A;
    int *IPIV;
 } pragma118;

typedef struct _pragma156 {
    double *Aij;
    double *prevSwap;
    int *dipiv;
    PLASMA_desc descA;
    int mintmp;
    int k;
    int m;
    int n;
    int tempk;
    int tempm;
    int tempkm;
    int tempkn;
    int tempmm;
    int tempnn;
    int ldak;
    int ldam;
    double zone;
    double mzone;
    void *fakedep;
    PLASMA_desc A;
    int *IPIV;
 } pragma156;

static void *pragma58_hclib_async(void *____arg);
static void *pragma81_hclib_async(void *____arg);
static void *pragma92_hclib_async(void *____arg);
static void *pragma118_hclib_async(void *____arg);
static void *pragma156_hclib_async(void *____arg);
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
pragma58 *ctx = (pragma58 *)malloc(sizeof(pragma58));
ctx->dA = dA;
ctx->dB = dB;
ctx->pDesc = pDesc;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->tempk = tempk;
ctx->tempm = tempm;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempmm = tempmm;
ctx->tempnn = tempnn;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->fakedep = fakedep;
ctx->A = A;
ctx->IPIV = IPIV;
hclib_emulate_omp_task(pragma58_hclib_async, ctx, ANY_PLACE, 1, 2, (dA) + (0), A.mb*A.nb, (dA) + (0), A.mb*A.nb, (dB) + (0), pDesc.n);
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
pragma81 *ctx = (pragma81 *)malloc(sizeof(pragma81));
ctx->descA = descA;
ctx->dA = dA;
ctx->dB = dB;
ctx->dipiv = dipiv;
ctx->pDesc = pDesc;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->tempk = tempk;
ctx->tempm = tempm;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempmm = tempmm;
ctx->tempnn = tempnn;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->fakedep = fakedep;
ctx->A = A;
ctx->IPIV = IPIV;
hclib_emulate_omp_task(pragma81_hclib_async, ctx, ANY_PLACE, 3, 1, (dA) + (0), 1, (dB) + (0), ldak, (dipiv) + (0), tempkm, (dA) + (0), 1);
 } ;

            m = k+1;
            if ( m < A.mt ) {
                tempmm = m == A.mt-1 ? A.m-m*A.mb : A.mb;
                ldam = BLKLDD(A, m);

                double *dA = A(m , k);
                double *dB = A(k , n);
                double *dC = A(m , n);
 { 
pragma92 *ctx = (pragma92 *)malloc(sizeof(pragma92));
ctx->dA = dA;
ctx->dB = dB;
ctx->dC = dC;
ctx->descA = descA;
ctx->dipiv = dipiv;
ctx->pDesc = pDesc;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->tempk = tempk;
ctx->tempm = tempm;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempmm = tempmm;
ctx->tempnn = tempnn;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->fakedep = fakedep;
ctx->A = A;
ctx->IPIV = IPIV;
hclib_emulate_omp_task(pragma92_hclib_async, ctx, ANY_PLACE, 3, 1, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (dC) + (0), A.mb*A.mb, (dC) + (0), A.mb*A.mb);
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
pragma118 *ctx = (pragma118 *)malloc(sizeof(pragma118));
ctx->dA = dA;
ctx->dB = dB;
ctx->dC = dC;
ctx->fake1 = fake1;
ctx->fake2 = fake2;
ctx->descA = descA;
ctx->dipiv = dipiv;
ctx->pDesc = pDesc;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->tempk = tempk;
ctx->tempm = tempm;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempmm = tempmm;
ctx->tempnn = tempnn;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->fakedep = fakedep;
ctx->A = A;
ctx->IPIV = IPIV;
hclib_emulate_omp_task(pragma118_hclib_async, ctx, ANY_PLACE, 5, 2, (dA) + (0), A.mb*A.mb, (dB) + (0), A.mb*A.mb, (fake2) + (0), 1, (dC) + (0), A.mb*A.mb, (fake1) + (0), A.mb*A.nb, (dC) + (0), A.mb*A.mb, (fake1) + (0), A.mb*A.nb);
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
pragma156 *ctx = (pragma156 *)malloc(sizeof(pragma156));
ctx->Aij = Aij;
ctx->prevSwap = prevSwap;
ctx->dipiv = dipiv;
ctx->descA = descA;
ctx->mintmp = mintmp;
ctx->k = k;
ctx->m = m;
ctx->n = n;
ctx->tempk = tempk;
ctx->tempm = tempm;
ctx->tempkm = tempkm;
ctx->tempkn = tempkn;
ctx->tempmm = tempmm;
ctx->tempnn = tempnn;
ctx->ldak = ldak;
ctx->ldam = ldam;
ctx->zone = zone;
ctx->mzone = mzone;
ctx->fakedep = fakedep;
ctx->A = A;
ctx->IPIV = IPIV;
hclib_emulate_omp_task(pragma156_hclib_async, ctx, ANY_PLACE, 4, 2, (Aij) + (0), 1, fakedep, 0, (dipiv) + (0), mintmp, (prevSwap) + (0), A.lm*A.nb, (Aij) + (0), 1, fakedep, 0);
 } ;
#endif
        }
    }
} 
static void *pragma58_hclib_async(void *____arg) {
    pragma58 *ctx = (pragma58 *)____arg;
    double *dA; dA = ctx->dA;
    int *dB; dB = ctx->dB;
    PLASMA_desc pDesc; pDesc = ctx->pDesc;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int tempk; tempk = ctx->tempk;
    int tempm; tempm = ctx->tempm;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempmm; tempmm = ctx->tempmm;
    int tempnn; tempnn = ctx->tempnn;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    void *fakedep; fakedep = ctx->fakedep;
    PLASMA_desc A; A = ctx->A;
    int *IPIV; IPIV = ctx->IPIV;
    hclib_start_finish();
{
            int info[3];
            info[1] = 0;
            info[2] = 1;

            CORE_dgetrf_rectil( pDesc, dB, info );
        } ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma81_hclib_async(void *____arg) {
    pragma81 *ctx = (pragma81 *)____arg;
    PLASMA_desc descA; descA = ctx->descA;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    int *dipiv; dipiv = ctx->dipiv;
    PLASMA_desc pDesc; pDesc = ctx->pDesc;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int tempk; tempk = ctx->tempk;
    int tempm; tempm = ctx->tempm;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempmm; tempmm = ctx->tempmm;
    int tempnn; tempnn = ctx->tempnn;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    void *fakedep; fakedep = ctx->fakedep;
    PLASMA_desc A; A = ctx->A;
    int *IPIV; IPIV = ctx->IPIV;
    hclib_start_finish();
CORE_dswptr_ontile(descA, 1, tempkm, dipiv, 1, dB, ldak) ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma92_hclib_async(void *____arg) {
    pragma92 *ctx = (pragma92 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    double *dC; dC = ctx->dC;
    PLASMA_desc descA; descA = ctx->descA;
    int *dipiv; dipiv = ctx->dipiv;
    PLASMA_desc pDesc; pDesc = ctx->pDesc;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int tempk; tempk = ctx->tempk;
    int tempm; tempm = ctx->tempm;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempmm; tempmm = ctx->tempmm;
    int tempnn; tempnn = ctx->tempnn;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    void *fakedep; fakedep = ctx->fakedep;
    PLASMA_desc A; A = ctx->A;
    int *IPIV; IPIV = ctx->IPIV;
    hclib_start_finish();
cblas_dgemm(CblasColMajor, (CBLAS_TRANSPOSE)PlasmaNoTrans, (CBLAS_TRANSPOSE)PlasmaNoTrans,
                        tempmm, tempnn, A.nb,
                        mzone, dA, ldam,
                        dB, ldak,
                        zone, dC, ldam) ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma118_hclib_async(void *____arg) {
    pragma118 *ctx = (pragma118 *)____arg;
    double *dA; dA = ctx->dA;
    double *dB; dB = ctx->dB;
    double *dC; dC = ctx->dC;
    double *fake1; fake1 = ctx->fake1;
    double *fake2; fake2 = ctx->fake2;
    PLASMA_desc descA; descA = ctx->descA;
    int *dipiv; dipiv = ctx->dipiv;
    PLASMA_desc pDesc; pDesc = ctx->pDesc;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int tempk; tempk = ctx->tempk;
    int tempm; tempm = ctx->tempm;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempmm; tempmm = ctx->tempmm;
    int tempnn; tempnn = ctx->tempnn;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    void *fakedep; fakedep = ctx->fakedep;
    PLASMA_desc A; A = ctx->A;
    int *IPIV; IPIV = ctx->IPIV;
    hclib_start_finish();
cblas_dgemm(CblasColMajor, (CBLAS_TRANSPOSE)PlasmaNoTrans, (CBLAS_TRANSPOSE)PlasmaNoTrans,
                                tempmm, tempnn, A.nb,
                                mzone, dA, ldam,
                                dB, ldak,
                                zone, dC, ldam) ;     ; hclib_end_finish();
    return NULL;
}


static void *pragma156_hclib_async(void *____arg) {
    pragma156 *ctx = (pragma156 *)____arg;
    double *Aij; Aij = ctx->Aij;
    double *prevSwap; prevSwap = ctx->prevSwap;
    int *dipiv; dipiv = ctx->dipiv;
    PLASMA_desc descA; descA = ctx->descA;
    int mintmp; mintmp = ctx->mintmp;
    int k; k = ctx->k;
    int m; m = ctx->m;
    int n; n = ctx->n;
    int tempk; tempk = ctx->tempk;
    int tempm; tempm = ctx->tempm;
    int tempkm; tempkm = ctx->tempkm;
    int tempkn; tempkn = ctx->tempkn;
    int tempmm; tempmm = ctx->tempmm;
    int tempnn; tempnn = ctx->tempnn;
    int ldak; ldak = ctx->ldak;
    int ldam; ldam = ctx->ldam;
    double zone; zone = ctx->zone;
    double mzone; mzone = ctx->mzone;
    void *fakedep; fakedep = ctx->fakedep;
    PLASMA_desc A; A = ctx->A;
    int *IPIV; IPIV = ctx->IPIV;
    hclib_start_finish();
CORE_dlaswp_ontile(descA, 1, mintmp, dipiv, 1) ;     ; hclib_end_finish();
    return NULL;
}


