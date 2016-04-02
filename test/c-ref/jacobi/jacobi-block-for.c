#include "hclib.h"
# include "poisson.h"

/* #pragma omp task/taskwait version of SWEEP. */
typedef struct _pragma20 {
    int nx;
    int ny;
    double dx;
    double dy;
    double *f_;
    int itold;
    int itnew;
    double *u_;
    double *unew_;
    int block_size;
    int it;
    int block_x;
    int block_y;
    int max_blocks_x;
    int max_blocks_y;
 } pragma20;

typedef struct _pragma25 {
    int nx;
    int ny;
    double dx;
    double dy;
    double *f_;
    int itold;
    int itnew;
    double *u_;
    double *unew_;
    int block_size;
    int it;
    int block_x;
    int block_y;
    int max_blocks_x;
    int max_blocks_y;
 } pragma25;

static void pragma20_hclib_async(void *____arg, const int ___iter0, const int ___iter1);
static void pragma25_hclib_async(void *____arg, const int ___iter0, const int ___iter1);
void sweep (int nx, int ny, double dx, double dy, double *f_,
        int itold, int itnew, double *u_, double *unew_, int block_size)
{
    int it;
    int block_x, block_y;

    if (block_size == 0)
        block_size = nx;

    int max_blocks_x = (nx / block_size);
    int max_blocks_y = (ny / block_size);
    
    for (it = itold + 1; it <= itnew; it++)
    {
        // Save the current estimate.
 { 
pragma20 *ctx = (pragma20 *)malloc(sizeof(pragma20));
ctx->nx = nx;
ctx->ny = ny;
ctx->dx = dx;
ctx->dy = dy;
ctx->f_ = f_;
ctx->itold = itold;
ctx->itnew = itnew;
ctx->u_ = u_;
ctx->unew_ = unew_;
ctx->block_size = block_size;
ctx->it = it;
ctx->block_x = block_x;
ctx->block_y = block_y;
ctx->max_blocks_x = max_blocks_x;
ctx->max_blocks_y = max_blocks_y;
hclib_loop_domain_t domain[2];
domain[0].low = 0;
domain[0].high = max_blocks_x;
domain[0].stride = 1;
domain[0].tile = 1;
domain[1].low = 0;
domain[1].high = max_blocks_y;
domain[1].stride = 1;
domain[1].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma20_hclib_async, ctx, NULL, 2, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } ;

 { 
pragma25 *ctx = (pragma25 *)malloc(sizeof(pragma25));
ctx->nx = nx;
ctx->ny = ny;
ctx->dx = dx;
ctx->dy = dy;
ctx->f_ = f_;
ctx->itold = itold;
ctx->itnew = itnew;
ctx->u_ = u_;
ctx->unew_ = unew_;
ctx->block_size = block_size;
ctx->it = it;
ctx->block_x = block_x;
ctx->block_y = block_y;
ctx->max_blocks_x = max_blocks_x;
ctx->max_blocks_y = max_blocks_y;
hclib_loop_domain_t domain[2];
domain[0].low = 0;
domain[0].high = max_blocks_x;
domain[0].stride = 1;
domain[0].tile = 1;
domain[1].low = 0;
domain[1].high = max_blocks_y;
domain[1].stride = 1;
domain[1].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma25_hclib_async, ctx, NULL, 2, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } ;
    }
} static void pragma20_hclib_async(void *____arg, const int ___iter0, const int ___iter1) {
    pragma20 *ctx = (pragma20 *)____arg;
    int nx; nx = ctx->nx;
    int ny; ny = ctx->ny;
    double dx; dx = ctx->dx;
    double dy; dy = ctx->dy;
    double *f_; f_ = ctx->f_;
    int itold; itold = ctx->itold;
    int itnew; itnew = ctx->itnew;
    double *u_; u_ = ctx->u_;
    double *unew_; unew_ = ctx->unew_;
    int block_size; block_size = ctx->block_size;
    int it; it = ctx->it;
    int block_x; block_x = ctx->block_x;
    int block_y; block_y = ctx->block_y;
    int max_blocks_x; max_blocks_x = ctx->max_blocks_x;
    int max_blocks_y; max_blocks_y = ctx->max_blocks_y;
    hclib_start_finish();
    do {
    block_x = ___iter0;
    block_y = ___iter1;
copy_block(nx, ny, block_x, block_y, u_, unew_, block_size) ;     } while (0);
    ; hclib_end_finish();
}

static void pragma25_hclib_async(void *____arg, const int ___iter0, const int ___iter1) {
    pragma25 *ctx = (pragma25 *)____arg;
    int nx; nx = ctx->nx;
    int ny; ny = ctx->ny;
    double dx; dx = ctx->dx;
    double dy; dy = ctx->dy;
    double *f_; f_ = ctx->f_;
    int itold; itold = ctx->itold;
    int itnew; itnew = ctx->itnew;
    double *u_; u_ = ctx->u_;
    double *unew_; unew_ = ctx->unew_;
    int block_size; block_size = ctx->block_size;
    int it; it = ctx->it;
    int block_x; block_x = ctx->block_x;
    int block_y; block_y = ctx->block_y;
    int max_blocks_x; max_blocks_x = ctx->max_blocks_x;
    int max_blocks_y; max_blocks_y = ctx->max_blocks_y;
    hclib_start_finish();
    do {
    block_x = ___iter0;
    block_y = ___iter1;
compute_estimate(block_x, block_y, u_, unew_, f_, dx, dy,
                                 nx, ny, block_size) ;     } while (0);
    ; hclib_end_finish();
}


