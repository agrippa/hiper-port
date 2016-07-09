#include "hclib.h"
#ifdef __cplusplus
#include "hclib_cpp.h"
#include "hclib_system.h"
#ifdef __CUDACC__
#include "hclib_cuda.h"
#endif
#endif
/**
 * @file ex_particle_OPENMP_seq.c
 * @author Michael Trotter & Matt Goodrum
 * @brief Particle filter implementation in C/OpenMP 
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#define PI 3.1415926535897932
/**
@var M value for Linear Congruential Generator (LCG); use GCC's value
*/
long M = INT_MAX;
/**
@var A value for LCG
*/
int A = 1103515245;
/**
@var C value for LCG
*/
int C = 12345;
/*****************************
*GET_TIME
*returns a long int representing the time
*****************************/
long long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}
// Returns the number of seconds elapsed between the two specified times
float elapsed_time(long long start_time, long long end_time) {
        return (float) (end_time - start_time) / (1000 * 1000);
}
/** 
* Takes in a double and returns an integer that approximates to that double
* @return if the mantissa < .5 => return value < input value; else return value > input value
*/
double roundDouble(double value){
	int newValue = (int)(value);
	if(value - newValue < .5)
	return newValue;
	else
	return newValue++;
}
/**
* Set values of the 3D array to a newValue if that value is equal to the testValue
* @param testValue The value to be replaced
* @param newValue The value to replace testValue with
* @param array3D The image vector
* @param dimX The x dimension of the frame
* @param dimY The y dimension of the frame
* @param dimZ The number of frames
*/
void setIf(int testValue, int newValue, int * array3D, int * dimX, int * dimY, int * dimZ){
	int x, y, z;
	for(x = 0; x < *dimX; x++){
		for(y = 0; y < *dimY; y++){
			for(z = 0; z < *dimZ; z++){
				if(array3D[x * *dimY * *dimZ+y * *dimZ + z] == testValue)
				array3D[x * *dimY * *dimZ + y * *dimZ + z] = newValue;
			}
		}
	}
}
/**
* Generates a uniformly distributed random number using the provided seed and GCC's settings for the Linear Congruential Generator (LCG)
* @see http://en.wikipedia.org/wiki/Linear_congruential_generator
* @note This function is thread-safe
* @param seed The seed array
* @param index The specific index of the seed to be advanced
* @return a uniformly distributed number [0, 1)
*/
double randu(int * seed, int index)
{
	int num = A*seed[index] + C;
	seed[index] = num % M;
	return fabs(seed[index]/((double) M));
}
/**
* Generates a normally distributed random number using the Box-Muller transformation
* @note This function is thread-safe
* @param seed The seed array
* @param index The specific index of the seed to be advanced
* @return a double representing random number generated using the Box-Muller algorithm
* @see http://en.wikipedia.org/wiki/Normal_distribution, section computing value for normal random distribution
*/
double randn(int * seed, int index){
	/*Box-Muller algorithm*/
	double u = randu(seed, index);
	double v = randu(seed, index);
	double cosine = cos(2*PI*v);
	double rt = -2*log(u);
	return sqrt(rt)*cosine;
}
/**
* Sets values of 3D matrix using randomly generated numbers from a normal distribution
* @param array3D The video to be modified
* @param dimX The x dimension of the frame
* @param dimY The y dimension of the frame
* @param dimZ The number of frames
* @param seed The seed array
*/
void addNoise(int * array3D, int * dimX, int * dimY, int * dimZ, int * seed){
	int x, y, z;
	for(x = 0; x < *dimX; x++){
		for(y = 0; y < *dimY; y++){
			for(z = 0; z < *dimZ; z++){
				array3D[x * *dimY * *dimZ + y * *dimZ + z] = array3D[x * *dimY * *dimZ + y * *dimZ + z] + (int)(5*randn(seed, 0));
			}
		}
	}
}
/**
* Fills a radius x radius matrix representing the disk
* @param disk The pointer to the disk to be made
* @param radius  The radius of the disk to be made
*/
void strelDisk(int * disk, int radius)
{
	int diameter = radius*2 - 1;
	int x, y;
	for(x = 0; x < diameter; x++){
		for(y = 0; y < diameter; y++){
			double distance = sqrt(pow((double)(x-radius+1),2) + pow((double)(y-radius+1),2));
			if(distance < radius)
			disk[x*diameter + y] = 1;
		}
	}
}
/**
* Dilates the provided video
* @param matrix The video to be dilated
* @param posX The x location of the pixel to be dilated
* @param posY The y location of the pixel to be dilated
* @param poxZ The z location of the pixel to be dilated
* @param dimX The x dimension of the frame
* @param dimY The y dimension of the frame
* @param dimZ The number of frames
* @param error The error radius
*/
void dilate_matrix(int * matrix, int posX, int posY, int posZ, int dimX, int dimY, int dimZ, int error)
{
	int startX = posX - error;
	while(startX < 0)
	startX++;
	int startY = posY - error;
	while(startY < 0)
	startY++;
	int endX = posX + error;
	while(endX > dimX)
	endX--;
	int endY = posY + error;
	while(endY > dimY)
	endY--;
	int x,y;
	for(x = startX; x < endX; x++){
		for(y = startY; y < endY; y++){
			double distance = sqrt( pow((double)(x-posX),2) + pow((double)(y-posY),2) );
			if(distance < error)
			matrix[x*dimY*dimZ + y*dimZ + posZ] = 1;
		}
	}
}

/**
* Dilates the target matrix using the radius as a guide
* @param matrix The reference matrix
* @param dimX The x dimension of the video
* @param dimY The y dimension of the video
* @param dimZ The z dimension of the video
* @param error The error radius to be dilated
* @param newMatrix The target matrix
*/
void imdilate_disk(int * matrix, int dimX, int dimY, int dimZ, int error, int * newMatrix)
{
	int x, y, z;
	for(z = 0; z < dimZ; z++){
		for(x = 0; x < dimX; x++){
			for(y = 0; y < dimY; y++){
				if(matrix[x*dimY*dimZ + y*dimZ + z] == 1){
					dilate_matrix(newMatrix, x, y, z, dimX, dimY, dimZ, error);
				}
			}
		}
	}
}
/**
* Fills a 2D array describing the offsets of the disk object
* @param se The disk object
* @param numOnes The number of ones in the disk
* @param neighbors The array that will contain the offsets
* @param radius The radius used for dilation
*/
void getneighbors(int * se, int numOnes, double * neighbors, int radius){
	int x, y;
	int neighY = 0;
	int center = radius - 1;
	int diameter = radius*2 -1;
	for(x = 0; x < diameter; x++){
		for(y = 0; y < diameter; y++){
			if(se[x*diameter + y]){
				neighbors[neighY*2] = (int)(y - center);
				neighbors[neighY*2 + 1] = (int)(x - center);
				neighY++;
			}
		}
	}
}
/**
* The synthetic video sequence we will work with here is composed of a
* single moving object, circular in shape (fixed radius)
* The motion here is a linear motion
* the foreground intensity and the backgrounf intensity is known
* the image is corrupted with zero mean Gaussian noise
* @param I The video itself
* @param IszX The x dimension of the video
* @param IszY The y dimension of the video
* @param Nfr The number of frames of the video
* @param seed The seed array used for number generation
*/
void videoSequence(int * I, int IszX, int IszY, int Nfr, int * seed){
	int k;
	int max_size = IszX*IszY*Nfr;
	/*get object centers*/
	int x0 = (int)roundDouble(IszY/2.0);
	int y0 = (int)roundDouble(IszX/2.0);
	I[x0 *IszY *Nfr + y0 * Nfr  + 0] = 1;
	
	/*move point*/
	int xk, yk, pos;
	for(k = 1; k < Nfr; k++){
		xk = abs(x0 + (k-1));
		yk = abs(y0 - 2*(k-1));
		pos = yk * IszY * Nfr + xk *Nfr + k;
		if(pos >= max_size)
		pos = 0;
		I[pos] = 1;
	}
	
	/*dilate matrix*/
	int * newMatrix = (int *)malloc(sizeof(int)*IszX*IszY*Nfr);
	imdilate_disk(I, IszX, IszY, Nfr, 5, newMatrix);
	int x, y;
	for(x = 0; x < IszX; x++){
		for(y = 0; y < IszY; y++){
			for(k = 0; k < Nfr; k++){
				I[x*IszY*Nfr + y*Nfr + k] = newMatrix[x*IszY*Nfr + y*Nfr + k];
			}
		}
	}
	free(newMatrix);
	
	/*define background, add noise*/
	setIf(0, 100, I, &IszX, &IszY, &Nfr);
	setIf(1, 228, I, &IszX, &IszY, &Nfr);
	/*add noise*/
	addNoise(I, &IszX, &IszY, &Nfr, seed);
}
/**
* Determines the likelihood sum based on the formula: SUM( (IK[IND] - 100)^2 - (IK[IND] - 228)^2)/ 100
* @param I The 3D matrix
* @param ind The current ind array
* @param numOnes The length of ind array
* @return A double representing the sum
*/
double calcLikelihoodSum(int * I, int * ind, int numOnes){
	double likelihoodSum = 0.0;
	int y;
	for(y = 0; y < numOnes; y++)
	likelihoodSum += (pow((I[ind[y]] - 100),2) - pow((I[ind[y]]-228),2))/50.0;
	return likelihoodSum;
}
/**
* Finds the first element in the CDF that is greater than or equal to the provided value and returns that index
* @note This function uses sequential search
* @param CDF The CDF
* @param lengthCDF The length of CDF
* @param value The value to be found
* @return The index of value in the CDF; if value is never found, returns the last index
*/
int findIndex(double * CDF, int lengthCDF, double value){
	int index = -1;
	int x;
	for(x = 0; x < lengthCDF; x++){
		if(CDF[x] >= value){
			index = x;
			break;
		}
	}
	if(index == -1){
		return lengthCDF-1;
	}
	return index;
}
/**
* Finds the first element in the CDF that is greater than or equal to the provided value and returns that index
* @note This function uses binary search before switching to sequential search
* @param CDF The CDF
* @param beginIndex The index to start searching from
* @param endIndex The index to stop searching
* @param value The value to find
* @return The index of value in the CDF; if value is never found, returns the last index
* @warning Use at your own risk; not fully tested
*/
int findIndexBin(double * CDF, int beginIndex, int endIndex, double value){
	if(endIndex < beginIndex)
	return -1;
	int middleIndex = beginIndex + ((endIndex - beginIndex)/2);
	/*check the value*/
	if(CDF[middleIndex] >= value)
	{
		/*check that it's good*/
		if(middleIndex == 0)
		return middleIndex;
		else if(CDF[middleIndex-1] < value)
		return middleIndex;
		else if(CDF[middleIndex-1] == value)
		{
			while(middleIndex > 0 && CDF[middleIndex-1] == value)
			middleIndex--;
			return middleIndex;
		}
	}
	if(CDF[middleIndex] > value)
	return findIndexBin(CDF, beginIndex, middleIndex+1, value);
	return findIndexBin(CDF, middleIndex-1, endIndex, value);
}
/**
* The implementation of the particle filter using OpenMP for many frames
* @see http://openmp.org/wp/
* @note This function is designed to work with a video of several frames. In addition, it references a provided MATLAB function which takes the video, the objxy matrix and the x and y arrays as arguments and returns the likelihoods
* @param I The video to be run
* @param IszX The x dimension of the video
* @param IszY The y dimension of the video
* @param Nfr The number of frames
* @param seed The seed array used for random number generation
* @param Nparticles The number of particles to be used
*/
typedef struct _pragma383_omp_parallel {
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma383_omp_parallel;

typedef struct _pragma398_omp_parallel {
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma398_omp_parallel;

typedef struct _pragma412_omp_parallel {
    long long (*set_arrays_ptr);
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma412_omp_parallel;

typedef struct _pragma420_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int y;
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int indX;
    int indY;
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma420_omp_parallel;

typedef struct _pragma443_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    long long (*likelihood_time_ptr);
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma443_omp_parallel;

typedef struct _pragma450_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    long long (*likelihood_time_ptr);
    long long (*exponential_ptr);
    double sumWeights;
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
    pthread_mutex_t reduction_mutex;
 } pragma450_omp_parallel;

typedef struct _pragma456_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    long long (*likelihood_time_ptr);
    long long (*exponential_ptr);
    double (*sumWeights_ptr);
    long long (*sum_time_ptr);
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma456_omp_parallel;

typedef struct _pragma465_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    long long (*likelihood_time_ptr);
    long long (*exponential_ptr);
    double (*sumWeights_ptr);
    long long (*sum_time_ptr);
    long long (*normalize_ptr);
    int (*max_size_ptr);
    long long (*start_ptr);
    double xe;
    double ye;
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
    pthread_mutex_t reduction_mutex;
 } pragma465_omp_parallel;

typedef struct _pragma490_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    long long (*likelihood_time_ptr);
    long long (*exponential_ptr);
    double (*sumWeights_ptr);
    long long (*sum_time_ptr);
    long long (*normalize_ptr);
    long long (*move_time_ptr);
    double (*distance_ptr);
    long long (*cum_sum_ptr);
    double (*u1_ptr);
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int x;
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma490_omp_parallel;

typedef struct _pragma498_omp_parallel {
    long long (*set_arrays_ptr);
    long long (*error_ptr);
    long long (*likelihood_time_ptr);
    long long (*exponential_ptr);
    double (*sumWeights_ptr);
    long long (*sum_time_ptr);
    long long (*normalize_ptr);
    long long (*move_time_ptr);
    double (*distance_ptr);
    long long (*cum_sum_ptr);
    double (*u1_ptr);
    long long (*u_time_ptr);
    int j;
    int i;
    int (*max_size_ptr);
    long long (*start_ptr);
    double (*xe_ptr);
    double (*ye_ptr);
    int (*radius_ptr);
    int (*diameter_ptr);
    int (*(*disk_ptr));
    int (*countOnes_ptr);
    int (*x_ptr);
    int (*y_ptr);
    double (*(*objxy_ptr));
    long long (*get_neighbors_ptr);
    double (*(*weights_ptr));
    long long (*get_weights_ptr);
    double (*(*likelihood_ptr));
    double (*(*arrayX_ptr));
    double (*(*arrayY_ptr));
    double (*(*xj_ptr));
    double (*(*yj_ptr));
    double (*(*CDF_ptr));
    double (*(*u_ptr));
    int (*(*ind_ptr));
    int (*k_ptr);
    int (*indX_ptr);
    int (*indY_ptr);
    int (*(*I_ptr));
    int (*IszX_ptr);
    int (*IszY_ptr);
    int (*Nfr_ptr);
    int (*(*seed_ptr));
    int (*Nparticles_ptr);
 } pragma498_omp_parallel;


#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma383_omp_parallel_hclib_async {
    private:
    double* volatile weights;
    int x;
    volatile int Nparticles;

    public:
        pragma383_omp_parallel_hclib_async(double* set_weights,
                int set_x,
                int set_Nparticles) {
            weights = set_weights;
            x = set_x;
            Nparticles = set_Nparticles;

        }

        __host__ __device__ void operator()(int x) {
            {
		weights[x] = 1/((double)(Nparticles));
	}
        }
};

#else
static void pragma383_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma398_omp_parallel_hclib_async {
    private:
    double* volatile arrayX;
    int x;
    volatile double xe;
    double* volatile arrayY;
    volatile double ye;

    public:
        pragma398_omp_parallel_hclib_async(double* set_arrayX,
                int set_x,
                double set_xe,
                double* set_arrayY,
                double set_ye) {
            arrayX = set_arrayX;
            x = set_x;
            xe = set_xe;
            arrayY = set_arrayY;
            ye = set_ye;

        }

        __host__ __device__ void operator()(int x) {
            {
		arrayX[x] = xe;
		arrayY[x] = ye;
	}
        }
};

#else
static void pragma398_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma412_omp_parallel_hclib_async {
    private:
        __device__ double randn(int * seed, int index) {
            {
	/*Box-Muller algorithm*/
	double u = randu(seed, index);
	double v = randu(seed, index);
	double cosine = cos(2*PI*v);
	double rt = -2*log(u);
	return sqrt(rt)*cosine;
}
        }
        __device__ double randn(int * seed, int index) {
            {
	/*Box-Muller algorithm*/
	double u = randu(seed, index);
	double v = randu(seed, index);
	double cosine = cos(2*PI*v);
	double rt = -2*log(u);
	return sqrt(rt)*cosine;
}
        }
    double* volatile arrayX;
    int x;
    int* volatile seed;
    double* volatile arrayY;

    public:
        pragma412_omp_parallel_hclib_async(double* set_arrayX,
                int set_x,
                int* set_seed,
                double* set_arrayY) {
            arrayX = set_arrayX;
            x = set_x;
            seed = set_seed;
            arrayY = set_arrayY;

        }

        __host__ __device__ void operator()(int x) {
            {
			arrayX[x] += 1 + 5*randn(seed, x);
			arrayY[x] += -2 + 2*randn(seed, x);
		}
        }
};

#else
static void pragma412_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma420_omp_parallel_hclib_async {
    private:
        __device__ double roundDouble(double value) {
            {
	int newValue = (int)(value);
	if(value - newValue < .5)
	return newValue;
	else
	return newValue++;
}
        }
        __device__ double roundDouble(double value) {
            {
	int newValue = (int)(value);
	if(value - newValue < .5)
	return newValue;
	else
	return newValue++;
}
        }
    int y;
    volatile int countOnes;
    int indX;
    double* volatile arrayX;
    int x;
    double* volatile objxy;
    int indY;
    double* volatile arrayY;
    int* volatile ind;
    volatile int IszY;
    volatile int Nfr;
    volatile int k;
    volatile int max_size;
    double* volatile likelihood;
    int* volatile I;

    public:
        pragma420_omp_parallel_hclib_async(int set_y,
                int set_countOnes,
                int set_indX,
                double* set_arrayX,
                int set_x,
                double* set_objxy,
                int set_indY,
                double* set_arrayY,
                int* set_ind,
                int set_IszY,
                int set_Nfr,
                int set_k,
                int set_max_size,
                double* set_likelihood,
                int* set_I) {
            y = set_y;
            countOnes = set_countOnes;
            indX = set_indX;
            arrayX = set_arrayX;
            x = set_x;
            objxy = set_objxy;
            indY = set_indY;
            arrayY = set_arrayY;
            ind = set_ind;
            IszY = set_IszY;
            Nfr = set_Nfr;
            k = set_k;
            max_size = set_max_size;
            likelihood = set_likelihood;
            I = set_I;

        }

        __host__ __device__ void operator()(int x) {
            {
			//compute the likelihood: remember our assumption is that you know
			// foreground and the background image intensity distribution.
			// Notice that we consider here a likelihood ratio, instead of
			// p(z|x). It is possible in this case. why? a hometask for you.		
			//calc ind
			for(y = 0; y < countOnes; y++){
				indX = roundDouble(arrayX[x]) + objxy[y*2 + 1];
				indY = roundDouble(arrayY[x]) + objxy[y*2];
				ind[x*countOnes + y] = fabs(indX*IszY*Nfr + indY*Nfr + k);
				if(ind[x*countOnes + y] >= max_size)
					ind[x*countOnes + y] = 0;
			}
			likelihood[x] = 0;
			for(y = 0; y < countOnes; y++)
				likelihood[x] += (pow((I[ind[x*countOnes + y]] - 100),2) - pow((I[ind[x*countOnes + y]]-228),2))/50.0;
			likelihood[x] = likelihood[x]/((double) countOnes);
		}
        }
};

#else
static void pragma420_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma443_omp_parallel_hclib_async {
    private:
    double* volatile weights;
    int x;
    double* volatile likelihood;

    public:
        pragma443_omp_parallel_hclib_async(double* set_weights,
                int set_x,
                double* set_likelihood) {
            weights = set_weights;
            x = set_x;
            likelihood = set_likelihood;

        }

        __host__ __device__ void operator()(int x) {
            {
			weights[x] = weights[x] * exp(likelihood[x]);
		}
        }
};

#else
static void pragma443_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma450_omp_parallel_hclib_async {
    private:
    double sumWeights;
    double* volatile weights;
    int x;

    public:
        pragma450_omp_parallel_hclib_async(double set_sumWeights,
                double* set_weights,
                int set_x) {
            sumWeights = set_sumWeights;
            weights = set_weights;
            x = set_x;

        }

        __host__ __device__ void operator()(int x) {
            {
			sumWeights += weights[x];
		}
        }
};

#else
static void pragma450_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma456_omp_parallel_hclib_async {
    private:
    double* volatile weights;
    int x;
    volatile double sumWeights;

    public:
        pragma456_omp_parallel_hclib_async(double* set_weights,
                int set_x,
                double set_sumWeights) {
            weights = set_weights;
            x = set_x;
            sumWeights = set_sumWeights;

        }

        __host__ __device__ void operator()(int x) {
            {
			weights[x] = weights[x]/sumWeights;
		}
        }
};

#else
static void pragma456_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma465_omp_parallel_hclib_async {
    private:
    double xe;
    double* volatile arrayX;
    int x;
    double* volatile weights;
    double ye;
    double* volatile arrayY;

    public:
        pragma465_omp_parallel_hclib_async(double set_xe,
                double* set_arrayX,
                int set_x,
                double* set_weights,
                double set_ye,
                double* set_arrayY) {
            xe = set_xe;
            arrayX = set_arrayX;
            x = set_x;
            weights = set_weights;
            ye = set_ye;
            arrayY = set_arrayY;

        }

        __host__ __device__ void operator()(int x) {
            {
			xe += arrayX[x] * weights[x];
			ye += arrayY[x] * weights[x];
		}
        }
};

#else
static void pragma465_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma490_omp_parallel_hclib_async {
    private:
    double* volatile u;
    int x;
    volatile double u1;
    volatile int Nparticles;

    public:
        pragma490_omp_parallel_hclib_async(double* set_u,
                int set_x,
                double set_u1,
                int set_Nparticles) {
            u = set_u;
            x = set_x;
            u1 = set_u1;
            Nparticles = set_Nparticles;

        }

        __host__ __device__ void operator()(int x) {
            {
			u[x] = u1 + x/((double)(Nparticles));
		}
        }
};

#else
static void pragma490_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif

#ifdef OMP_TO_HCLIB_ENABLE_GPU

class pragma498_omp_parallel_hclib_async {
    private:
        __device__ int findIndex(double * CDF, int lengthCDF, double value) {
            {
	int index = -1;
	int x;
	for(x = 0; x < lengthCDF; x++){
		if(CDF[x] >= value){
			index = x;
			break;
		}
	}
	if(index == -1){
		return lengthCDF-1;
	}
	return index;
}
        }
    int i;
    double* volatile CDF;
    volatile int Nparticles;
    double* volatile u;
    int j;
    double* volatile xj;
    double* volatile arrayX;
    double* volatile yj;
    double* volatile arrayY;

    public:
        pragma498_omp_parallel_hclib_async(int set_i,
                double* set_CDF,
                int set_Nparticles,
                double* set_u,
                int set_j,
                double* set_xj,
                double* set_arrayX,
                double* set_yj,
                double* set_arrayY) {
            i = set_i;
            CDF = set_CDF;
            Nparticles = set_Nparticles;
            u = set_u;
            j = set_j;
            xj = set_xj;
            arrayX = set_arrayX;
            yj = set_yj;
            arrayY = set_arrayY;

        }

        __host__ __device__ void operator()(int j) {
            {
			i = findIndex(CDF, Nparticles, u[j]);
			if(i == -1)
				i = Nparticles-1;
			xj[j] = arrayX[i];
			yj[j] = arrayY[i];
			
		}
        }
};

#else
static void pragma498_omp_parallel_hclib_async(void *____arg, const int ___iter0);
#endif
void particleFilter(int * I, int IszX, int IszY, int Nfr, int * seed, int Nparticles){
	
	int max_size = IszX*IszY*Nfr;
	long long start = get_time();
	//original particle centroid
	double xe = roundDouble(IszY/2.0);
	double ye = roundDouble(IszX/2.0);
	
	//expected object locations, compared to center
	int radius = 5;
	int diameter = radius*2 - 1;
	int * disk = (int *)malloc(diameter*diameter*sizeof(int));
	strelDisk(disk, radius);
	int countOnes = 0;
	int x, y;
	for(x = 0; x < diameter; x++){
		for(y = 0; y < diameter; y++){
			if(disk[x*diameter + y] == 1)
				countOnes++;
		}
	}
	double * objxy = (double *)malloc(countOnes*2*sizeof(double));
	getneighbors(disk, countOnes, objxy, radius);
	
	long long get_neighbors = get_time();
	printf("TIME TO GET NEIGHBORS TOOK: %f\n", elapsed_time(start, get_neighbors));
	//initial weights are all equal (1/Nparticles)
	double * weights = (double *)malloc(sizeof(double)*Nparticles);
 { 
pragma383_omp_parallel *new_ctx = (pragma383_omp_parallel *)malloc(sizeof(pragma383_omp_parallel));
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma383_omp_parallel_hclib_async(weights, x, Nparticles), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma383_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
	long long get_weights = get_time();
	printf("TIME TO GET WEIGHTSTOOK: %f\n", elapsed_time(get_neighbors, get_weights));
	//initial likelihood to 0.0
	double * likelihood = (double *)malloc(sizeof(double)*Nparticles);
	double * arrayX = (double *)malloc(sizeof(double)*Nparticles);
	double * arrayY = (double *)malloc(sizeof(double)*Nparticles);
	double * xj = (double *)malloc(sizeof(double)*Nparticles);
	double * yj = (double *)malloc(sizeof(double)*Nparticles);
	double * CDF = (double *)malloc(sizeof(double)*Nparticles);
	double * u = (double *)malloc(sizeof(double)*Nparticles);
	int * ind = (int*)malloc(sizeof(int)*countOnes*Nparticles);
 { 
pragma398_omp_parallel *new_ctx = (pragma398_omp_parallel *)malloc(sizeof(pragma398_omp_parallel));
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma398_omp_parallel_hclib_async(arrayX, x, xe, arrayY, ye), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma398_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
	int k;
	
	printf("TIME TO SET ARRAYS TOOK: %f\n", elapsed_time(get_weights, get_time()));
	int indX, indY;
	for(k = 1; k < Nfr; k++){
		long long set_arrays = get_time();
		//apply motion model
		//draws sample from motion model (random walk). The only prior information
		//is that the object moves 2x as fast as in the y direction
 { 
pragma412_omp_parallel *new_ctx = (pragma412_omp_parallel *)malloc(sizeof(pragma412_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma412_omp_parallel_hclib_async(arrayX, x, seed, arrayY), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma412_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
		long long error = get_time();
		printf("TIME TO SET ERROR TOOK: %f\n", elapsed_time(set_arrays, error));
		//particle filter likelihood
 { 
pragma420_omp_parallel *new_ctx = (pragma420_omp_parallel *)malloc(sizeof(pragma420_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y = y;
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX = indX;
new_ctx->indY = indY;
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma420_omp_parallel_hclib_async(y, countOnes, indX, arrayX, x, objxy, indY, arrayY, ind, IszY, Nfr, k, max_size, likelihood, I), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma420_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
		long long likelihood_time = get_time();
		printf("TIME TO GET LIKELIHOODS TOOK: %f\n", elapsed_time(error, likelihood_time));
		// update & normalize weights
		// using equation (63) of Arulampalam Tutorial
 { 
pragma443_omp_parallel *new_ctx = (pragma443_omp_parallel *)malloc(sizeof(pragma443_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->likelihood_time_ptr = &(likelihood_time);
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma443_omp_parallel_hclib_async(weights, x, likelihood), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma443_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
		long long exponential = get_time();
		printf("TIME TO GET EXP TOOK: %f\n", elapsed_time(likelihood_time, exponential));
		double sumWeights = 0;
 { 
pragma450_omp_parallel *new_ctx = (pragma450_omp_parallel *)malloc(sizeof(pragma450_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->likelihood_time_ptr = &(likelihood_time);
new_ctx->exponential_ptr = &(exponential);
new_ctx->sumWeights = sumWeights;
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
new_ctx->sumWeights = 0;
const int init_err = pthread_mutex_init(&new_ctx->reduction_mutex, NULL);
assert(init_err == 0);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma450_omp_parallel_hclib_async(sumWeights, weights, x), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma450_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
sumWeights = new_ctx->sumWeights;
 } 
		long long sum_time = get_time();
		printf("TIME TO SUM WEIGHTS TOOK: %f\n", elapsed_time(exponential, sum_time));
 { 
pragma456_omp_parallel *new_ctx = (pragma456_omp_parallel *)malloc(sizeof(pragma456_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->likelihood_time_ptr = &(likelihood_time);
new_ctx->exponential_ptr = &(exponential);
new_ctx->sumWeights_ptr = &(sumWeights);
new_ctx->sum_time_ptr = &(sum_time);
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma456_omp_parallel_hclib_async(weights, x, sumWeights), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma456_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
		long long normalize = get_time();
		printf("TIME TO NORMALIZE WEIGHTS TOOK: %f\n", elapsed_time(sum_time, normalize));
		xe = 0;
		ye = 0;
		// estimate the object location by expected values
 { 
pragma465_omp_parallel *new_ctx = (pragma465_omp_parallel *)malloc(sizeof(pragma465_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->likelihood_time_ptr = &(likelihood_time);
new_ctx->exponential_ptr = &(exponential);
new_ctx->sumWeights_ptr = &(sumWeights);
new_ctx->sum_time_ptr = &(sum_time);
new_ctx->normalize_ptr = &(normalize);
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe = xe;
new_ctx->ye = ye;
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
new_ctx->xe = 0;
new_ctx->ye = 0;
const int init_err = pthread_mutex_init(&new_ctx->reduction_mutex, NULL);
assert(init_err == 0);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma465_omp_parallel_hclib_async(xe, arrayX, x, weights, ye, arrayY), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma465_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
xe = new_ctx->xe;
ye = new_ctx->ye;
 } 
		long long move_time = get_time();
		printf("TIME TO MOVE OBJECT TOOK: %f\n", elapsed_time(normalize, move_time));
		printf("XE: %lf\n", xe);
		printf("YE: %lf\n", ye);
		double distance = sqrt( pow((double)(xe-(int)roundDouble(IszY/2.0)),2) + pow((double)(ye-(int)roundDouble(IszX/2.0)),2) );
		printf("%lf\n", distance);
		//display(hold off for now)
		
		//pause(hold off for now)
		
		//resampling
		
		
		CDF[0] = weights[0];
		for(x = 1; x < Nparticles; x++){
			CDF[x] = weights[x] + CDF[x-1];
		}
		long long cum_sum = get_time();
		printf("TIME TO CALC CUM SUM TOOK: %f\n", elapsed_time(move_time, cum_sum));
		double u1 = (1/((double)(Nparticles)))*randu(seed, 0);
 { 
pragma490_omp_parallel *new_ctx = (pragma490_omp_parallel *)malloc(sizeof(pragma490_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->likelihood_time_ptr = &(likelihood_time);
new_ctx->exponential_ptr = &(exponential);
new_ctx->sumWeights_ptr = &(sumWeights);
new_ctx->sum_time_ptr = &(sum_time);
new_ctx->normalize_ptr = &(normalize);
new_ctx->move_time_ptr = &(move_time);
new_ctx->distance_ptr = &(distance);
new_ctx->cum_sum_ptr = &(cum_sum);
new_ctx->u1_ptr = &(u1);
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x = x;
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma490_omp_parallel_hclib_async(u, x, u1, Nparticles), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma490_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
		long long u_time = get_time();
		printf("TIME TO CALC U TOOK: %f\n", elapsed_time(cum_sum, u_time));
		int j, i;
		
 { 
pragma498_omp_parallel *new_ctx = (pragma498_omp_parallel *)malloc(sizeof(pragma498_omp_parallel));
new_ctx->set_arrays_ptr = &(set_arrays);
new_ctx->error_ptr = &(error);
new_ctx->likelihood_time_ptr = &(likelihood_time);
new_ctx->exponential_ptr = &(exponential);
new_ctx->sumWeights_ptr = &(sumWeights);
new_ctx->sum_time_ptr = &(sum_time);
new_ctx->normalize_ptr = &(normalize);
new_ctx->move_time_ptr = &(move_time);
new_ctx->distance_ptr = &(distance);
new_ctx->cum_sum_ptr = &(cum_sum);
new_ctx->u1_ptr = &(u1);
new_ctx->u_time_ptr = &(u_time);
new_ctx->j = j;
new_ctx->i = i;
new_ctx->max_size_ptr = &(max_size);
new_ctx->start_ptr = &(start);
new_ctx->xe_ptr = &(xe);
new_ctx->ye_ptr = &(ye);
new_ctx->radius_ptr = &(radius);
new_ctx->diameter_ptr = &(diameter);
new_ctx->disk_ptr = &(disk);
new_ctx->countOnes_ptr = &(countOnes);
new_ctx->x_ptr = &(x);
new_ctx->y_ptr = &(y);
new_ctx->objxy_ptr = &(objxy);
new_ctx->get_neighbors_ptr = &(get_neighbors);
new_ctx->weights_ptr = &(weights);
new_ctx->get_weights_ptr = &(get_weights);
new_ctx->likelihood_ptr = &(likelihood);
new_ctx->arrayX_ptr = &(arrayX);
new_ctx->arrayY_ptr = &(arrayY);
new_ctx->xj_ptr = &(xj);
new_ctx->yj_ptr = &(yj);
new_ctx->CDF_ptr = &(CDF);
new_ctx->u_ptr = &(u);
new_ctx->ind_ptr = &(ind);
new_ctx->k_ptr = &(k);
new_ctx->indX_ptr = &(indX);
new_ctx->indY_ptr = &(indY);
new_ctx->I_ptr = &(I);
new_ctx->IszX_ptr = &(IszX);
new_ctx->IszY_ptr = &(IszY);
new_ctx->Nfr_ptr = &(Nfr);
new_ctx->seed_ptr = &(seed);
new_ctx->Nparticles_ptr = &(Nparticles);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = -1;
#ifdef OMP_TO_HCLIB_ENABLE_GPU
hclib::future_t *fut = hclib::forasync_cuda((Nparticles) - (0), pragma498_omp_parallel_hclib_async(i, CDF, Nparticles, u, j, xj, arrayX, yj, arrayY), hclib::get_closest_gpu_locale(), NULL);
fut->wait();
#else
hclib_future_t *fut = hclib_forasync_future((void *)pragma498_omp_parallel_hclib_async, new_ctx, 1, domain, HCLIB_FORASYNC_MODE);
hclib_future_wait(fut);
#endif
free(new_ctx);
 } 
		long long xyj_time = get_time();
		printf("TIME TO CALC NEW ARRAY X AND Y TOOK: %f\n", elapsed_time(u_time, xyj_time));
		
		//#pragma omp parallel for shared(weights, Nparticles) private(x)
		for(x = 0; x < Nparticles; x++){
			//reassign arrayX and arrayY
			arrayX[x] = xj[x];
			arrayY[x] = yj[x];
			weights[x] = 1/((double)(Nparticles));
		}
		long long reset = get_time();
		printf("TIME TO RESET WEIGHTS TOOK: %f\n", elapsed_time(xyj_time, reset));
	}
	free(disk);
	free(objxy);
	free(weights);
	free(likelihood);
	free(xj);
	free(yj);
	free(arrayX);
	free(arrayY);
	free(CDF);
	free(u);
	free(ind);
} 

#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma383_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma383_omp_parallel *ctx = (pragma383_omp_parallel *)____arg;
    int x; x = ctx->x;
    do {
    x = ___iter0;
{
		(*(ctx->weights_ptr))[x] = 1/((double)((*(ctx->Nparticles_ptr))));
	} ;     } while (0);
}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma398_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma398_omp_parallel *ctx = (pragma398_omp_parallel *)____arg;
    int x; x = ctx->x;
    do {
    x = ___iter0;
{
		(*(ctx->arrayX_ptr))[x] = (*(ctx->xe_ptr));
		(*(ctx->arrayY_ptr))[x] = (*(ctx->ye_ptr));
	} ;     } while (0);
}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma412_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma412_omp_parallel *ctx = (pragma412_omp_parallel *)____arg;
    int x; x = ctx->x;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			(*(ctx->arrayX_ptr))[x] += 1 + 5*randn((*(ctx->seed_ptr)), x);
			(*(ctx->arrayY_ptr))[x] += -2 + 2*randn((*(ctx->seed_ptr)), x);
		} ;     } while (0);
    ; hclib_end_finish_nonblocking();

}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma420_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma420_omp_parallel *ctx = (pragma420_omp_parallel *)____arg;
    int x; x = ctx->x;
    int y; y = ctx->y;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			//compute the likelihood: remember our assumption is that you know
			// foreground and the background image intensity distribution.
			// Notice that we consider here a likelihood ratio, instead of
			// p(z|x). It is possible in this case. why? a hometask for you.		
			//calc ind
			for(y = 0; y < (*(ctx->countOnes_ptr)); y++){
				indX = roundDouble((*(ctx->arrayX_ptr))[x]) + (*(ctx->objxy_ptr))[y*2 + 1];
				indY = roundDouble((*(ctx->arrayY_ptr))[x]) + (*(ctx->objxy_ptr))[y*2];
				(*(ctx->ind_ptr))[x*(*(ctx->countOnes_ptr)) + y] = fabs(indX*(*(ctx->IszY_ptr))*(*(ctx->Nfr_ptr)) + indY*(*(ctx->Nfr_ptr)) + (*(ctx->k_ptr)));
				if((*(ctx->ind_ptr))[x*(*(ctx->countOnes_ptr)) + y] >= (*(ctx->max_size_ptr)))
					(*(ctx->ind_ptr))[x*(*(ctx->countOnes_ptr)) + y] = 0;
			}
			(*(ctx->likelihood_ptr))[x] = 0;
			for(y = 0; y < (*(ctx->countOnes_ptr)); y++)
				(*(ctx->likelihood_ptr))[x] += (pow(((*(ctx->I_ptr))[(*(ctx->ind_ptr))[x*(*(ctx->countOnes_ptr)) + y]] - 100),2) - pow(((*(ctx->I_ptr))[(*(ctx->ind_ptr))[x*(*(ctx->countOnes_ptr)) + y]]-228),2))/50.0;
			(*(ctx->likelihood_ptr))[x] = (*(ctx->likelihood_ptr))[x]/((double) (*(ctx->countOnes_ptr)));
		} ;     } while (0);
    ; hclib_end_finish_nonblocking();

}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma443_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma443_omp_parallel *ctx = (pragma443_omp_parallel *)____arg;
    int x; x = ctx->x;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			(*(ctx->weights_ptr))[x] = (*(ctx->weights_ptr))[x] * exp((*(ctx->likelihood_ptr))[x]);
		} ;     } while (0);
    ; hclib_end_finish_nonblocking();

}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma450_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma450_omp_parallel *ctx = (pragma450_omp_parallel *)____arg;
    double sumWeights; sumWeights = ctx->sumWeights;
    int x; x = ctx->x;
    do {
    x = ___iter0;
{
			sumWeights += (*(ctx->weights_ptr))[x];
		} ;     } while (0);
    const int lock_err = pthread_mutex_lock(&ctx->reduction_mutex);
    assert(lock_err == 0);
    ctx->sumWeights += sumWeights;
    const int unlock_err = pthread_mutex_unlock(&ctx->reduction_mutex);
    assert(unlock_err == 0);
}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma456_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma456_omp_parallel *ctx = (pragma456_omp_parallel *)____arg;
    int x; x = ctx->x;
    do {
    x = ___iter0;
{
			(*(ctx->weights_ptr))[x] = (*(ctx->weights_ptr))[x]/(*(ctx->sumWeights_ptr));
		} ;     } while (0);
}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma465_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma465_omp_parallel *ctx = (pragma465_omp_parallel *)____arg;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int x; x = ctx->x;
    do {
    x = ___iter0;
{
			xe += (*(ctx->arrayX_ptr))[x] * (*(ctx->weights_ptr))[x];
			ye += (*(ctx->arrayY_ptr))[x] * (*(ctx->weights_ptr))[x];
		} ;     } while (0);
    const int lock_err = pthread_mutex_lock(&ctx->reduction_mutex);
    assert(lock_err == 0);
    ctx->xe += xe;
    ctx->ye += ye;
    const int unlock_err = pthread_mutex_unlock(&ctx->reduction_mutex);
    assert(unlock_err == 0);
}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma490_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma490_omp_parallel *ctx = (pragma490_omp_parallel *)____arg;
    int x; x = ctx->x;
    do {
    x = ___iter0;
{
			(*(ctx->u_ptr))[x] = (*(ctx->u1_ptr)) + x/((double)((*(ctx->Nparticles_ptr))));
		} ;     } while (0);
}

#endif


#ifndef OMP_TO_HCLIB_ENABLE_GPU

static void pragma498_omp_parallel_hclib_async(void *____arg, const int ___iter0) {
    pragma498_omp_parallel *ctx = (pragma498_omp_parallel *)____arg;
    int j; j = ctx->j;
    int i; i = ctx->i;
    hclib_start_finish();
    do {
    j = ___iter0;
{
			i = findIndex((*(ctx->CDF_ptr)), (*(ctx->Nparticles_ptr)), (*(ctx->u_ptr))[j]);
			if(i == -1)
				i = (*(ctx->Nparticles_ptr))-1;
			(*(ctx->xj_ptr))[j] = (*(ctx->arrayX_ptr))[i];
			(*(ctx->yj_ptr))[j] = (*(ctx->arrayY_ptr))[i];
			
		} ;     } while (0);
    ; hclib_end_finish_nonblocking();

}

#endif

typedef struct _main_entrypoint_ctx {
    char (*usage);
    int IszX;
    int IszY;
    int Nfr;
    int Nparticles;
    int (*seed);
    int i;
    int (*I);
    long long start;
    long long endVideoSequence;
    int argc;
    char (*(*argv));
 } main_entrypoint_ctx;


static void main_entrypoint(void *____arg) {
    main_entrypoint_ctx *ctx = (main_entrypoint_ctx *)____arg;
    char (*usage); usage = ctx->usage;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int Nparticles; Nparticles = ctx->Nparticles;
    int (*seed); seed = ctx->seed;
    int i; i = ctx->i;
    int (*I); I = ctx->I;
    long long start; start = ctx->start;
    long long endVideoSequence; endVideoSequence = ctx->endVideoSequence;
    int argc; argc = ctx->argc;
    char (*(*argv)); argv = ctx->argv;
particleFilter(I, IszX, IszY, Nfr, seed, Nparticles) ;     free(____arg);
}

int main(int argc, char * argv[]){
	
	char* usage = "openmp.out -x <dimX> -y <dimY> -z <Nfr> -np <Nparticles>";
	//check number of arguments
	if(argc != 9)
	{
		printf("%s\n", usage);
		return 0;
	}
	//check args deliminators
	if( strcmp( argv[1], "-x" ) ||  strcmp( argv[3], "-y" ) || strcmp( argv[5], "-z" ) || strcmp( argv[7], "-np" ) ) {
		printf( "%s\n",usage );
		return 0;
	}
	
	int IszX, IszY, Nfr, Nparticles;
	
	//converting a string to a integer
	if( sscanf( argv[2], "%d", &IszX ) == EOF ) {
	   printf("ERROR: dimX input is incorrect");
	   return 0;
	}
	
	if( IszX <= 0 ) {
		printf("dimX must be > 0\n");
		return 0;
	}
	
	//converting a string to a integer
	if( sscanf( argv[4], "%d", &IszY ) == EOF ) {
	   printf("ERROR: dimY input is incorrect");
	   return 0;
	}
	
	if( IszY <= 0 ) {
		printf("dimY must be > 0\n");
		return 0;
	}
	
	//converting a string to a integer
	if( sscanf( argv[6], "%d", &Nfr ) == EOF ) {
	   printf("ERROR: Number of frames input is incorrect");
	   return 0;
	}
	
	if( Nfr <= 0 ) {
		printf("number of frames must be > 0\n");
		return 0;
	}
	
	//converting a string to a integer
	if( sscanf( argv[8], "%d", &Nparticles ) == EOF ) {
	   printf("ERROR: Number of particles input is incorrect");
	   return 0;
	}
	
	if( Nparticles <= 0 ) {
		printf("Number of particles must be > 0\n");
		return 0;
	}
	//establish seed
	int * seed = (int *)malloc(sizeof(int)*Nparticles);
	int i;
	for(i = 0; i < Nparticles; i++)
		seed[i] = time(0)*i;
	//malloc matrix
	int * I = (int *)malloc(sizeof(int)*IszX*IszY*Nfr);
	long long start = get_time();
	//call video sequence
	videoSequence(I, IszX, IszY, Nfr, seed);
	long long endVideoSequence = get_time();
	printf("VIDEO SEQUENCE TOOK %f\n", elapsed_time(start, endVideoSequence));
	//call particle filter
main_entrypoint_ctx *new_ctx = (main_entrypoint_ctx *)malloc(sizeof(main_entrypoint_ctx));
new_ctx->usage = usage;
new_ctx->IszX = IszX;
new_ctx->IszY = IszY;
new_ctx->Nfr = Nfr;
new_ctx->Nparticles = Nparticles;
new_ctx->seed = seed;
new_ctx->i = i;
new_ctx->I = I;
new_ctx->start = start;
new_ctx->endVideoSequence = endVideoSequence;
new_ctx->argc = argc;
new_ctx->argv = argv;
const char *deps[] = { "system" };
hclib_launch(main_entrypoint, new_ctx, deps, 1);
;

	long long endParticleFilter = get_time();
	printf("PARTICLE FILTER TOOK %f\n", elapsed_time(endVideoSequence, endParticleFilter));
	printf("ENTIRE PROGRAM TOOK %f\n", elapsed_time(start, endParticleFilter));
	
	free(seed);
	free(I);
	return 0;
} 
