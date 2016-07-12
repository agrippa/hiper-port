#include "hclib.h"
int ____num_tasks[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
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

hclib_pragma_marker("omp", "parallel shared(u_, unew_, f_, max_blocks_x, max_blocks_y, nx, ny, dx, dy, itold, itnew, block_size) private(it, block_x, block_y)", "pragma21_omp_parallel");
    {
hclib_pragma_marker("omp", "single", "pragma23_omp_single");
    {
        for (it = itold + 1; it <= itnew; it++) {
            // Save the current estimate.
            for (block_x = 0; block_x < max_blocks_x; block_x++) {
                for (block_y = 0; block_y < max_blocks_y; block_y++) {
hclib_pragma_marker("omp", "task shared(u_, unew_, nx, ny, block_size) firstprivate(block_x, block_y)", "pragma29_omp_task");
                    copy_block(nx, ny, block_x, block_y, u_, unew_, block_size);
                }
            }

hclib_pragma_marker("omp", "taskwait", "pragma34_omp_taskwait");

            // Compute a new estimate.
            for (block_x = 0; block_x < max_blocks_x; block_x++) {
                for (block_y = 0; block_y < max_blocks_y; block_y++) {
hclib_pragma_marker("omp", "task default(none) shared(u_, unew_, f_, dx, dy, nx, ny, block_size) firstprivate(block_x, block_y)", "pragma39_omp_task");
                    compute_estimate(block_x, block_y, u_, unew_, f_, dx, dy,
                                     nx, ny, block_size);
                }
            }

hclib_pragma_marker("omp", "taskwait", "pragma45_omp_taskwait");
        }
    }
    }
}
