#include "hclib.h"
int ____num_tasks[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
# include "poisson.h"

/* #pragma omp task/taskwait version of SWEEP. */
void sweep (int nx, int ny, double dx, double dy, double *f,
        int itold, int itnew, double *u, double *unew, int block_size)
{
    int i;
    int it;
    int j;
    // double (*f)[nx][ny] = (double (*)[nx][ny])f_;
    // double (*u)[nx][ny] = (double (*)[nx][ny])u_;
    // double (*unew)[nx][ny] = (double (*)[nx][ny])unew_;

hclib_pragma_marker("omp", "parallel shared (f, u, unew) private (i, it, j) firstprivate(nx, ny, dx, dy, itold, itnew)", "pragma17_omp_parallel");
    {
hclib_pragma_marker("omp", "single", "pragma19_omp_single");
    {
        for (it = itold + 1; it <= itnew; it++) {
            // Save the current estimate.
            for (i = 0; i < nx; i++) {
hclib_pragma_marker("omp", "task firstprivate(i, ny) private(j) shared(u, unew)", "pragma24_omp_task");
                for (j = 0; j < ny; j++) {
                    (u)[i * ny + j] = (unew)[i * ny + j];
                }
            }
hclib_pragma_marker("omp", "taskwait", "pragma29_omp_taskwait");
            // Compute a new estimate.
            for (i = 0; i < nx; i++) {
hclib_pragma_marker("omp", "task firstprivate(i, dx, dy, nx, ny) private(j) shared(u, unew, f)", "pragma32_omp_task");
                for (j = 0; j < ny; j++) {
                    if (i == 0 || j == 0 || i == nx - 1 || j == ny - 1) {
                        (unew)[i * ny + j] = (f)[i * ny + j];
                    } else {
                        (unew)[i * ny + j] = 0.25 * ((u)[(i-1) * ny + j] + (u)[i * ny + (j+1)]
                                                + (u)[i * ny + (j-1)] + (u)[(i+1) * ny + j]
                                                + (f)[i * ny + j] * dx * dy);
                    }
                }
            }
hclib_pragma_marker("omp", "taskwait", "pragma43_omp_taskwait");
        }
    }
    }
}
