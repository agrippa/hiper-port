#include "hclib.h"
# include "poisson.h"

/* #pragma omp task/taskwait version of SWEEP. */
typedef struct _pragma22 {
    int nx;
    int ny;
    double dx;
    double dy;
    double *f;
    int itold;
    int itnew;
    double *u;
    double *unew;
    int block_size;
    int i;
    int it;
    int j;
 } pragma22;

typedef struct _pragma30 {
    int nx;
    int ny;
    double dx;
    double dy;
    double *f;
    int itold;
    int itnew;
    double *u;
    double *unew;
    int block_size;
    int i;
    int it;
    int j;
 } pragma30;

static void pragma22_hclib_async(void *____arg);
static void pragma30_hclib_async(void *____arg);
void sweep (int nx, int ny, double dx, double dy, double *f,
        int itold, int itnew, double *u, double *unew, int block_size)
{
    int i;
    int it;
    int j;
    // double (*f)[nx][ny] = (double (*)[nx][ny])f_;
    // double (*u)[nx][ny] = (double (*)[nx][ny])u_;
    // double (*unew)[nx][ny] = (double (*)[nx][ny])unew_;

hclib_start_finish(); {
        for (it = itold + 1; it <= itnew; it++) {
            // Save the current estimate.
            for (i = 0; i < nx; i++) {
 { 
pragma22 *ctx = (pragma22 *)malloc(sizeof(pragma22));
ctx->nx = nx;
ctx->ny = ny;
ctx->dx = dx;
ctx->dy = dy;
ctx->f = f;
ctx->itold = itold;
ctx->itnew = itnew;
ctx->u = u;
ctx->unew = unew;
ctx->block_size = block_size;
ctx->i = i;
ctx->it = it;
ctx->j = j;
hclib_async(pragma22_hclib_async, ctx, NO_FUTURE, ANY_PLACE);
 } 
            }
 hclib_end_finish(); hclib_start_finish(); ;
            // Compute a new estimate.
            for (i = 0; i < nx; i++) {
 { 
pragma30 *ctx = (pragma30 *)malloc(sizeof(pragma30));
ctx->nx = nx;
ctx->ny = ny;
ctx->dx = dx;
ctx->dy = dy;
ctx->f = f;
ctx->itold = itold;
ctx->itnew = itnew;
ctx->u = u;
ctx->unew = unew;
ctx->block_size = block_size;
ctx->i = i;
ctx->it = it;
ctx->j = j;
hclib_async(pragma30_hclib_async, ctx, NO_FUTURE, ANY_PLACE);
 } 
            }
 hclib_end_finish(); hclib_start_finish(); ;
        }
    } ; hclib_end_finish(); 
} static void pragma22_hclib_async(void *____arg) {
    pragma22 *ctx = (pragma22 *)____arg;
    int nx; nx = ctx->nx;
    int ny; ny = ctx->ny;
    double dx; dx = ctx->dx;
    double dy; dy = ctx->dy;
    double *f; f = ctx->f;
    int itold; itold = ctx->itold;
    int itnew; itnew = ctx->itnew;
    double *u; u = ctx->u;
    double *unew; unew = ctx->unew;
    int block_size; block_size = ctx->block_size;
    int i; i = ctx->i;
    int it; it = ctx->it;
    int j; j = ctx->j;
    hclib_start_finish();
for (j = 0; j < ny; j++) {
                    (u)[i * ny + j] = (unew)[i * ny + j];
                } ;     ; hclib_end_finish();
}

static void pragma30_hclib_async(void *____arg) {
    pragma30 *ctx = (pragma30 *)____arg;
    int nx; nx = ctx->nx;
    int ny; ny = ctx->ny;
    double dx; dx = ctx->dx;
    double dy; dy = ctx->dy;
    double *f; f = ctx->f;
    int itold; itold = ctx->itold;
    int itnew; itnew = ctx->itnew;
    double *u; u = ctx->u;
    double *unew; unew = ctx->unew;
    int block_size; block_size = ctx->block_size;
    int i; i = ctx->i;
    int it; it = ctx->it;
    int j; j = ctx->j;
    hclib_start_finish();
for (j = 0; j < ny; j++) {
                    if (i == 0 || j == 0 || i == nx - 1 || j == ny - 1) {
                        (unew)[i * ny + j] = (f)[i * ny + j];
                    } else {
                        (unew)[i * ny + j] = 0.25 * ((u)[(i-1) * ny + j] + (u)[i * ny + (j+1)]
                                                + (u)[i * ny + (j-1)] + (u)[(i+1) * ny + j]
                                                + (f)[i * ny + j] * dx * dy);
                    }
                } ;     ; hclib_end_finish();
}


