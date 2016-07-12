#include "hclib.h"
# include "poisson.h"

/* #pragma omp task/taskwait version of SWEEP. */
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
unsigned long long ____hclib_start_time = hclib_current_time_ns();
#pragma omp parallel for shared(u_, unew_, f_, max_blocks_x, max_blocks_y, nx, ny, dx, dy, itold, itnew, block_size) private(it, block_x, block_y) collapse(2)
for (block_x = 0; block_x < max_blocks_x; block_x++)
            for (block_y = 0; block_y < max_blocks_y; block_y++)
                copy_block(nx, ny, block_x, block_y, u_, unew_, block_size) ; unsigned long long ____hclib_end_time = hclib_current_time_ns(); printf("pragma21_omp_parallel %llu ns\n", ____hclib_end_time - ____hclib_start_time);;

unsigned long long ____hclib_start_time = hclib_current_time_ns();
#pragma omp parallel for shared(u_, unew_, f_, max_blocks_x, max_blocks_y, nx, ny, dx, dy, itold, itnew, block_size) private(it, block_x, block_y) collapse(2)
for (block_x = 0; block_x < max_blocks_x; block_x++)
            for (block_y = 0; block_y < max_blocks_y; block_y++)
                compute_estimate(block_x, block_y, u_, unew_, f_, dx, dy,
                                 nx, ny, block_size) ; unsigned long long ____hclib_end_time = hclib_current_time_ns(); printf("pragma26_omp_parallel %llu ns\n", ____hclib_end_time - ____hclib_start_time);;
    }
}
