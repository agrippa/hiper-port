#include <sys/time.h>
#include <time.h>
#include <stdio.h>
static unsigned long long current_time_ns() {
#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    unsigned long long s = 1000000000ULL * (unsigned long long)mts.tv_sec;
    return (unsigned long long)mts.tv_nsec + s;
#else
    struct timespec t ={0,0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    unsigned long long s = 1000000000ULL * (unsigned long long)t.tv_sec;
    return (((unsigned long long)t.tv_nsec)) + s;
#endif
}
#include "shmem.h"
#include <assert.h>

int main(int argc, char **argv) {
    shmem_init();
    int MyRank = shmem_my_pe ();
    int Numprocs = shmem_n_pes ();
    printf("rank = %d size = %d\n", MyRank, Numprocs);

    int *shared_arr = (int *)shmem_malloc(10 * sizeof(int));
    assert(shared_arr);

    int i;
    for (i = 0; i < 10; i++) {
        shared_arr[i] = 3;
    }

    shmem_finalize();
    return 0;
}
