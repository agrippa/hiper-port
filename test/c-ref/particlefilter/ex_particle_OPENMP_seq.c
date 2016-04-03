#include "hclib.h"
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
typedef struct _pragma373 {
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma373;

typedef struct _pragma388 {
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma388;

typedef struct _pragma402 {
    long long set_arrays;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma402;

typedef struct _pragma410 {
    long long set_arrays;
    long long error;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma410;

typedef struct _pragma433 {
    long long set_arrays;
    long long error;
    long long likelihood_time;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma433;

typedef struct _pragma440 {
    long long set_arrays;
    long long error;
    long long likelihood_time;
    long long exponential;
    double sumWeights;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
    pthread_mutex_t reduction_mutex;
 } pragma440;

typedef struct _pragma446 {
    long long set_arrays;
    long long error;
    long long likelihood_time;
    long long exponential;
    double sumWeights;
    long long sum_time;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma446;

typedef struct _pragma455 {
    long long set_arrays;
    long long error;
    long long likelihood_time;
    long long exponential;
    double sumWeights;
    long long sum_time;
    long long normalize;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
    pthread_mutex_t reduction_mutex;
 } pragma455;

typedef struct _pragma480 {
    long long set_arrays;
    long long error;
    long long likelihood_time;
    long long exponential;
    double sumWeights;
    long long sum_time;
    long long normalize;
    long long move_time;
    double distance;
    long long cum_sum;
    double u1;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma480;

typedef struct _pragma488 {
    long long set_arrays;
    long long error;
    long long likelihood_time;
    long long exponential;
    double sumWeights;
    long long sum_time;
    long long normalize;
    long long move_time;
    double distance;
    long long cum_sum;
    double u1;
    long long u_time;
    int j;
    int i;
    int max_size;
    long long start;
    double xe;
    double ye;
    int radius;
    int diameter;
    int *disk;
    int countOnes;
    int x;
    int y;
    double *objxy;
    long long get_neighbors;
    double *weights;
    long long get_weights;
    double *likelihood;
    double *arrayX;
    double *arrayY;
    double *xj;
    double *yj;
    double *CDF;
    double *u;
    int *ind;
    int k;
    int indX;
    int indY;
    int *I;
    int IszX;
    int IszY;
    int Nfr;
    int *seed;
    int Nparticles;
 } pragma488;

static void pragma373_hclib_async(void *____arg, const int ___iter0);
static void pragma388_hclib_async(void *____arg, const int ___iter0);
static void pragma402_hclib_async(void *____arg, const int ___iter0);
static void pragma410_hclib_async(void *____arg, const int ___iter0);
static void pragma433_hclib_async(void *____arg, const int ___iter0);
static void pragma440_hclib_async(void *____arg, const int ___iter0);
static void pragma446_hclib_async(void *____arg, const int ___iter0);
static void pragma455_hclib_async(void *____arg, const int ___iter0);
static void pragma480_hclib_async(void *____arg, const int ___iter0);
static void pragma488_hclib_async(void *____arg, const int ___iter0);
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
pragma373 *ctx = (pragma373 *)malloc(sizeof(pragma373));
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma373_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
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
pragma388 *ctx = (pragma388 *)malloc(sizeof(pragma388));
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma388_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
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
pragma402 *ctx = (pragma402 *)malloc(sizeof(pragma402));
ctx->set_arrays = set_arrays;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma402_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } 
		long long error = get_time();
		printf("TIME TO SET ERROR TOOK: %f\n", elapsed_time(set_arrays, error));
		//particle filter likelihood
 { 
pragma410 *ctx = (pragma410 *)malloc(sizeof(pragma410));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma410_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } 
		long long likelihood_time = get_time();
		printf("TIME TO GET LIKELIHOODS TOOK: %f\n", elapsed_time(error, likelihood_time));
		// update & normalize weights
		// using equation (63) of Arulampalam Tutorial
 { 
pragma433 *ctx = (pragma433 *)malloc(sizeof(pragma433));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->likelihood_time = likelihood_time;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma433_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } 
		long long exponential = get_time();
		printf("TIME TO GET EXP TOOK: %f\n", elapsed_time(likelihood_time, exponential));
		double sumWeights = 0;
 { 
pragma440 *ctx = (pragma440 *)malloc(sizeof(pragma440));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->likelihood_time = likelihood_time;
ctx->exponential = exponential;
ctx->sumWeights = sumWeights;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
ctx->sumWeights = 0;
const int init_err = pthread_mutex_init(&ctx->reduction_mutex, NULL);
assert(init_err == 0);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma440_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
sumWeights = ctx->sumWeights;
 } 
		long long sum_time = get_time();
		printf("TIME TO SUM WEIGHTS TOOK: %f\n", elapsed_time(exponential, sum_time));
 { 
pragma446 *ctx = (pragma446 *)malloc(sizeof(pragma446));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->likelihood_time = likelihood_time;
ctx->exponential = exponential;
ctx->sumWeights = sumWeights;
ctx->sum_time = sum_time;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma446_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } 
		long long normalize = get_time();
		printf("TIME TO NORMALIZE WEIGHTS TOOK: %f\n", elapsed_time(sum_time, normalize));
		xe = 0;
		ye = 0;
		// estimate the object location by expected values
 { 
pragma455 *ctx = (pragma455 *)malloc(sizeof(pragma455));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->likelihood_time = likelihood_time;
ctx->exponential = exponential;
ctx->sumWeights = sumWeights;
ctx->sum_time = sum_time;
ctx->normalize = normalize;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
ctx->xe = 0;
ctx->ye = 0;
const int init_err = pthread_mutex_init(&ctx->reduction_mutex, NULL);
assert(init_err == 0);
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma455_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
xe = ctx->xe;
ye = ctx->ye;
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
pragma480 *ctx = (pragma480 *)malloc(sizeof(pragma480));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->likelihood_time = likelihood_time;
ctx->exponential = exponential;
ctx->sumWeights = sumWeights;
ctx->sum_time = sum_time;
ctx->normalize = normalize;
ctx->move_time = move_time;
ctx->distance = distance;
ctx->cum_sum = cum_sum;
ctx->u1 = u1;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma480_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
 } 
		long long u_time = get_time();
		printf("TIME TO CALC U TOOK: %f\n", elapsed_time(cum_sum, u_time));
		int j, i;
		
 { 
pragma488 *ctx = (pragma488 *)malloc(sizeof(pragma488));
ctx->set_arrays = set_arrays;
ctx->error = error;
ctx->likelihood_time = likelihood_time;
ctx->exponential = exponential;
ctx->sumWeights = sumWeights;
ctx->sum_time = sum_time;
ctx->normalize = normalize;
ctx->move_time = move_time;
ctx->distance = distance;
ctx->cum_sum = cum_sum;
ctx->u1 = u1;
ctx->u_time = u_time;
ctx->j = j;
ctx->i = i;
ctx->max_size = max_size;
ctx->start = start;
ctx->xe = xe;
ctx->ye = ye;
ctx->radius = radius;
ctx->diameter = diameter;
ctx->disk = disk;
ctx->countOnes = countOnes;
ctx->x = x;
ctx->y = y;
ctx->objxy = objxy;
ctx->get_neighbors = get_neighbors;
ctx->weights = weights;
ctx->get_weights = get_weights;
ctx->likelihood = likelihood;
ctx->arrayX = arrayX;
ctx->arrayY = arrayY;
ctx->xj = xj;
ctx->yj = yj;
ctx->CDF = CDF;
ctx->u = u;
ctx->ind = ind;
ctx->k = k;
ctx->indX = indX;
ctx->indY = indY;
ctx->I = I;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->seed = seed;
ctx->Nparticles = Nparticles;
hclib_loop_domain_t domain[1];
domain[0].low = 0;
domain[0].high = Nparticles;
domain[0].stride = 1;
domain[0].tile = 1;
hclib_future_t *fut = hclib_forasync_future((void *)pragma488_hclib_async, ctx, NULL, 1, domain, FORASYNC_MODE_RECURSIVE);
hclib_future_wait(fut);
free(ctx);
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
static void pragma373_hclib_async(void *____arg, const int ___iter0) {
    pragma373 *ctx = (pragma373 *)____arg;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
		weights[x] = 1/((double)(Nparticles));
	} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma388_hclib_async(void *____arg, const int ___iter0) {
    pragma388 *ctx = (pragma388 *)____arg;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
		arrayX[x] = xe;
		arrayY[x] = ye;
	} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma402_hclib_async(void *____arg, const int ___iter0) {
    pragma402 *ctx = (pragma402 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			arrayX[x] += 1 + 5*randn(seed, x);
			arrayY[x] += -2 + 2*randn(seed, x);
		} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma410_hclib_async(void *____arg, const int ___iter0) {
    pragma410 *ctx = (pragma410 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
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
		} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma433_hclib_async(void *____arg, const int ___iter0) {
    pragma433 *ctx = (pragma433 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    long long likelihood_time; likelihood_time = ctx->likelihood_time;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			weights[x] = weights[x] * exp(likelihood[x]);
		} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma440_hclib_async(void *____arg, const int ___iter0) {
    pragma440 *ctx = (pragma440 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    long long likelihood_time; likelihood_time = ctx->likelihood_time;
    long long exponential; exponential = ctx->exponential;
    double sumWeights; sumWeights = ctx->sumWeights;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			sumWeights += weights[x];
		} ;     } while (0);
    const int lock_err = pthread_mutex_lock(&ctx->reduction_mutex);
    assert(lock_err == 0);
    ctx->sumWeights += sumWeights;
    const int unlock_err = pthread_mutex_unlock(&ctx->reduction_mutex);
    assert(unlock_err == 0);
    ; hclib_end_finish();
}


static void pragma446_hclib_async(void *____arg, const int ___iter0) {
    pragma446 *ctx = (pragma446 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    long long likelihood_time; likelihood_time = ctx->likelihood_time;
    long long exponential; exponential = ctx->exponential;
    double sumWeights; sumWeights = ctx->sumWeights;
    long long sum_time; sum_time = ctx->sum_time;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			weights[x] = weights[x]/sumWeights;
		} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma455_hclib_async(void *____arg, const int ___iter0) {
    pragma455 *ctx = (pragma455 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    long long likelihood_time; likelihood_time = ctx->likelihood_time;
    long long exponential; exponential = ctx->exponential;
    double sumWeights; sumWeights = ctx->sumWeights;
    long long sum_time; sum_time = ctx->sum_time;
    long long normalize; normalize = ctx->normalize;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			xe += arrayX[x] * weights[x];
			ye += arrayY[x] * weights[x];
		} ;     } while (0);
    const int lock_err = pthread_mutex_lock(&ctx->reduction_mutex);
    assert(lock_err == 0);
    ctx->xe += xe;
    ctx->ye += ye;
    const int unlock_err = pthread_mutex_unlock(&ctx->reduction_mutex);
    assert(unlock_err == 0);
    ; hclib_end_finish();
}


static void pragma480_hclib_async(void *____arg, const int ___iter0) {
    pragma480 *ctx = (pragma480 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    long long likelihood_time; likelihood_time = ctx->likelihood_time;
    long long exponential; exponential = ctx->exponential;
    double sumWeights; sumWeights = ctx->sumWeights;
    long long sum_time; sum_time = ctx->sum_time;
    long long normalize; normalize = ctx->normalize;
    long long move_time; move_time = ctx->move_time;
    double distance; distance = ctx->distance;
    long long cum_sum; cum_sum = ctx->cum_sum;
    double u1; u1 = ctx->u1;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    x = ___iter0;
{
			u[x] = u1 + x/((double)(Nparticles));
		} ;     } while (0);
    ; hclib_end_finish();
}


static void pragma488_hclib_async(void *____arg, const int ___iter0) {
    pragma488 *ctx = (pragma488 *)____arg;
    long long set_arrays; set_arrays = ctx->set_arrays;
    long long error; error = ctx->error;
    long long likelihood_time; likelihood_time = ctx->likelihood_time;
    long long exponential; exponential = ctx->exponential;
    double sumWeights; sumWeights = ctx->sumWeights;
    long long sum_time; sum_time = ctx->sum_time;
    long long normalize; normalize = ctx->normalize;
    long long move_time; move_time = ctx->move_time;
    double distance; distance = ctx->distance;
    long long cum_sum; cum_sum = ctx->cum_sum;
    double u1; u1 = ctx->u1;
    long long u_time; u_time = ctx->u_time;
    int j; j = ctx->j;
    int i; i = ctx->i;
    int max_size; max_size = ctx->max_size;
    long long start; start = ctx->start;
    double xe; xe = ctx->xe;
    double ye; ye = ctx->ye;
    int radius; radius = ctx->radius;
    int diameter; diameter = ctx->diameter;
    int *disk; disk = ctx->disk;
    int countOnes; countOnes = ctx->countOnes;
    int x; x = ctx->x;
    int y; y = ctx->y;
    double *objxy; objxy = ctx->objxy;
    long long get_neighbors; get_neighbors = ctx->get_neighbors;
    double *weights; weights = ctx->weights;
    long long get_weights; get_weights = ctx->get_weights;
    double *likelihood; likelihood = ctx->likelihood;
    double *arrayX; arrayX = ctx->arrayX;
    double *arrayY; arrayY = ctx->arrayY;
    double *xj; xj = ctx->xj;
    double *yj; yj = ctx->yj;
    double *CDF; CDF = ctx->CDF;
    double *u; u = ctx->u;
    int *ind; ind = ctx->ind;
    int k; k = ctx->k;
    int indX; indX = ctx->indX;
    int indY; indY = ctx->indY;
    int *I; I = ctx->I;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int *seed; seed = ctx->seed;
    int Nparticles; Nparticles = ctx->Nparticles;
    hclib_start_finish();
    do {
    j = ___iter0;
{
			i = findIndex(CDF, Nparticles, u[j]);
			if(i == -1)
				i = Nparticles-1;
			xj[j] = arrayX[i];
			yj[j] = arrayY[i];
			
		} ;     } while (0);
    ; hclib_end_finish();
}


typedef struct _main_entrypoint_ctx {
    char *usage;
    int IszX;
    int IszY;
    int Nfr;
    int Nparticles;
    int *seed;
    int i;
    int *I;
    long long start;
    long long endVideoSequence;
    int argc;
    char **argv;
 } main_entrypoint_ctx;


static void main_entrypoint(void *____arg) {
    main_entrypoint_ctx *ctx = (main_entrypoint_ctx *)____arg;
    char *usage; usage = ctx->usage;
    int IszX; IszX = ctx->IszX;
    int IszY; IszY = ctx->IszY;
    int Nfr; Nfr = ctx->Nfr;
    int Nparticles; Nparticles = ctx->Nparticles;
    int *seed; seed = ctx->seed;
    int i; i = ctx->i;
    int *I; I = ctx->I;
    long long start; start = ctx->start;
    long long endVideoSequence; endVideoSequence = ctx->endVideoSequence;
    int argc; argc = ctx->argc;
    char **argv; argv = ctx->argv;
particleFilter(I, IszX, IszY, Nfr, seed, Nparticles) ; }

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
main_entrypoint_ctx *ctx = (main_entrypoint_ctx *)malloc(sizeof(main_entrypoint_ctx));
ctx->usage = usage;
ctx->IszX = IszX;
ctx->IszY = IszY;
ctx->Nfr = Nfr;
ctx->Nparticles = Nparticles;
ctx->seed = seed;
ctx->i = i;
ctx->I = I;
ctx->start = start;
ctx->endVideoSequence = endVideoSequence;
ctx->argc = argc;
ctx->argv = argv;
hclib_launch(main_entrypoint, ctx);
free(ctx);
;

	long long endParticleFilter = get_time();
	printf("PARTICLE FILTER TOOK %f\n", elapsed_time(endVideoSequence, endParticleFilter));
	printf("ENTIRE PROGRAM TOOK %f\n", elapsed_time(start, endParticleFilter));
	
	free(seed);
	free(I);
	return 0;
} 
