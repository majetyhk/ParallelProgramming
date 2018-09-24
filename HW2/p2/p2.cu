#include<stdio.h>
#include<cuda_runtime.h>
#include<math.h>
#include<curand_kernel.h>

#define SEED 35791246

__global__ void setup_kernel(curandState *state, int numElements)
{
    int id = threadIdx.x + blockIdx.x * blockDim.x;
    /* Each thread gets same seed, a different sequence 
       number, no offset */
    if(id<numElements){
    	curand_init(SEED, id, 0, &state[id]);
    }
    
}


__global__ void getMonteCarloVal(float *dX,float *dY, float *dZ, curandState *state, int numElements){
	int idx = blockDim.x * blockIdx.x + threadIdx.x;

	if(idx<numElements){
		//dX[idx] = rand()/(float)RAND_MAX;
		//dY[idx] = rand()/(float)RAND_MAX;
	    //curandGenerator_t prng;
	    //curandCreateGenerator(&prng, CURAND_RNG_PSEUDO_DEFAULT);
	    //curandSetPseudoRandomGeneratorSeed(prng, SEED);
		//curandGenerateUniform(prng, dX+idx, 1);
		//curandGenerateUniform(prng, dY+idx, 1);
		curandState localState = state[idx];
		dX[idx] = curand_uniform(&localState);
		dY[idx] = curand_uniform(&localState);
		dZ[idx] = ((dX[idx]*dX[idx] + dY[idx]*dY[idx])<=1?1:0);
		state[idx] = localState;
		//curandDestroyGenerator(prng);
	}
}


int main(int argc, char** argv){
	//cudaError_t err = cudaSuccess;

	int numElements = atoi(argv[1]);
	size_t size = numElements * sizeof(float);
	float *dX, *dY, *dZ, *hZ;
	cudaMalloc((void **)&dX, size);
	cudaMalloc((void **)&dY, size);
	cudaMalloc((void **)&dZ, size);
	hZ = (float *)malloc(size);
	curandState *devStates;
	cudaMalloc((void **)&devStates, numElements * sizeof(curandState));
	//Cuda Configuration
	int threadsPerBlock = 256;
	int nBlocks = (numElements+threadsPerBlock-1)/threadsPerBlock;
	setup_kernel<<<nBlocks,threadsPerBlock>>>(devStates,numElements);
	getMonteCarloVal<<<nBlocks,threadsPerBlock>>>(dX, dY, dZ, devStates, numElements);
	cudaMemcpy(hZ, dZ, numElements, cudaMemcpyDeviceToHost);
	int count = 0;
	for (int i = 0; i < numElements; ++i)
	{
		count=count+hZ[i];
		printf("%f\n", hZ[i]);
	}
	printf("%d\n",count);
	printf("The approximate value of Pi is %f\n", ((float)count/numElements)*4 );
	cudaFree(dX);
	cudaFree(dY);
	cudaFree(dZ);
	free(hZ);
	cudaDeviceReset();
	return 0;
}