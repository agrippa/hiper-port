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
// #ifdef __cplusplus
// extern "C" {
// #endif

//========================================================================================================================================================================================================200
//	DEFINE/INCLUDE
//========================================================================================================================================================================================================200

//======================================================================================================================================================150
//	LIBRARIES
//======================================================================================================================================================150

#include <omp.h>									// (in directory known to compiler)			needed by openmp
#include <stdlib.h>									// (in directory known to compiler)			needed by malloc
#include <stdio.h>									// (in directory known to compiler)			needed by printf, stderr
#include <assert.h>

//======================================================================================================================================================150
//	COMMON
//======================================================================================================================================================150

#include "common.h"								// (in directory provided here)

//======================================================================================================================================================150
//	UTILITIES
//======================================================================================================================================================150

#include "timer.h"					// (in directory provided here)

//========================================================================================================================================================================================================200
//	KERNEL_CPU FUNCTION
//========================================================================================================================================================================================================200

void 
kernel_cpu(	int cores_arg,

			record *records,
			knode *knodes,
			long knodes_elem,

			int order,
			long maxheight,
			int count,

			long *currKnode,
			long *offset,
			int *keys,
			record *ans)
{

	//======================================================================================================================================================150
	//	Variables
	//======================================================================================================================================================150

	// timer
	long long time0;
	long long time1;
	long long time2;

	time0 = get_time();

	//======================================================================================================================================================150
	//	MCPU SETUP
	//======================================================================================================================================================150

	int threadsPerBlock;
	threadsPerBlock = order < 1024 ? order : 1024;

    {

	time1 = get_time();

	//======================================================================================================================================================150
	//	PROCESS INTERACTIONS
	//======================================================================================================================================================150

	// private thread IDs
	int thid;
	int bid;
	int i;

	// process number of querries
 { const unsigned long long parallel_for_start = current_time_ns();
#pragma omp parallel for private (i, thid)
for(bid = 0; bid < count; bid++){

		// process levels of the tree
		for(i = 0; i < maxheight; i++){

			// process all leaves at each level
			for(thid = 0; thid < threadsPerBlock; thid++){

				// if value is between the two keys
				if((knodes[currKnode[bid]].keys[thid]) <= keys[bid] && (knodes[currKnode[bid]].keys[thid+1] > keys[bid])){
					// this conditional statement is inserted to avoid crush due to but in original code
					// "offset[bid]" calculated below that addresses knodes[] in the next iteration goes outside of its bounds cause segmentation fault
					// more specifically, values saved into knodes->indices in the main function are out of bounds of knodes that they address
					if(knodes[offset[bid]].indices[thid] < knodes_elem){
						offset[bid] = knodes[offset[bid]].indices[thid];
					}
				}

			}

			// set for next tree level
			currKnode[bid] = offset[bid];

		}

		//At this point, we have a candidate leaf node which may contain
		//the target record.  Check each key to hopefully find the record
		// process all leaves at each level
		for(thid = 0; thid < threadsPerBlock; thid++){

			if(knodes[currKnode[bid]].keys[thid] == keys[bid]){
				ans[bid].value = records[knodes[currKnode[bid]].indices[thid]].value;
			}

		}

	} ; 
const unsigned long long parallel_for_end = current_time_ns();
printf("pragma83_omp_parallel %llu ns", parallel_for_end - parallel_for_start); } 


	time2 = get_time();
    }

	//======================================================================================================================================================150
	//	DISPLAY TIMING
	//======================================================================================================================================================150

	printf("Time spent in different stages of CPU/MCPU KERNEL:\n");

	printf("%15.12f s, %15.12f % : MCPU: SET DEVICE\n",					(float) (time1-time0) / 1000000, (float) (time1-time0) / (float) (time2-time0) * 100);
	printf("%15.12f s, %15.12f % : CPU/MCPU: KERNEL\n",					(float) (time2-time1) / 1000000, (float) (time2-time1) / (float) (time2-time0) * 100);

	printf("Total time:\n");
	printf("%.12f s\n", 												(float) (time2-time0) / 1000000);

}

//========================================================================================================================================================================================================200
//	END
//========================================================================================================================================================================================================200

// #ifdef __cplusplus
// }
// #endif

