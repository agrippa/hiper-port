#include "hclib.h"
#ifdef __cplusplus
#include "hclib_cpp.h"
#include "hclib_system.h"
#include "hclib_openshmem.h"
#endif
#include "shmem.h"
#include <assert.h>

int main(int argc, char **argv) {
    ;
    int MyRank = hclib::shmem_my_pe ();
    int Numprocs = hclib::shmem_n_pes ();
    printf("rank = %d size = %d\n", MyRank, Numprocs);

    int *shared_arr = (int *)hclib::shmem_malloc(10 * sizeof(int));
    assert(shared_arr);

    int i;
    for (i = 0; i < 10; i++) {
        shared_arr[i] = 3;
    }

    ;
    return 0;
}
