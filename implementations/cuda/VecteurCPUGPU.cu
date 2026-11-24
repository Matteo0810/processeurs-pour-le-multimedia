#include <stdio.h>
#include <sys/time.h>

#define InitTimerGPU 			\
	cudaEvent_t start, stop; 	\
  	float elapsedTime;		\
 	cudaEventCreate(&start);	\
 	cudaEventCreate(&stop);	
 
#define StartTimerGPU	{	cudaEventRecord(start,0);   }

#define StopTimerGPU	{		\
 	cudaEventRecord(stop,0);	\
 	cudaEventSynchronize(stop);	\
 	cudaEventElapsedTime(&elapsedTime, start,stop); }
 	
#define PrintExecutionTimeGPU { 	\
	printf("Vecteur %d ==> Temps calcul GPU %.2f us\n" ,tailleVecteur, elapsedTime*1000);}



#define InitTimerCPU struct timeval tv1, tv2; struct timezone tz
#define StartTimerCPU gettimeofday(&tv1, &tz)
#define StopTimerCPU gettimeofday(&tv2, &tz)
#define PrintExecutionTimeCPU 	printf("Vecteur %d => Temps calcul CPU %ld us \n", tailleVecteur, (tv2.tv_sec-tv1.tv_sec)*1000000L + (tv2.tv_usec-tv1.tv_usec));


int tailleVecteur ;


/* KERNEL CUDA */

__global__ void add_vec_scalaire_gpu(int *vec, int *res, int a, long N) {
	long i = (long)blockIdx.x * (long)blockDim.x + (long)threadIdx.x;
	if (i < N) {
		res[i] = vec[i] + a;
	}
}


// Fonction CPU

void add_vec_scalaire_cpu(int *vec, int *res, int a, int N) 
{
	int i ;
	for (i=0 ; i < N ; i ++) {
		res[i] = vec[i] + a;
	}
}

int main(int argc, char *argv[]) {

InitTimerGPU;
InitTimerCPU;

	int alpha = 10;
	int *cudaVec;
	int *cudaRes;
	int i ;
	long size = 0;
	
	if (argc < 2) {
		printf("Erreur, manque un argument\n");
		exit(0);
	}
	tailleVecteur = atol(argv[1]);
	int vecteur[tailleVecteur];
	int resultat[tailleVecteur];
	long blocksize = 1;	
	if (argc ==3) {
		blocksize = atoi(argv[2]);
	}
	
	tailleVecteur = atoi(argv[1]);

	size = sizeof(int)*tailleVecteur;

	for (i= 0 ; i < tailleVecteur ; i++) {
		vecteur[i] = rand() % 100;
		resultat[i] = 0;
	}

	if (cudaMalloc((void **)&cudaVec, size) == cudaErrorMemoryAllocation) {
		printf("Allocation memoire qui pose probleme (cudaVec) \n");
	}
	if (cudaMalloc((void **)&cudaRes, size)  == cudaErrorMemoryAllocation) {
		printf("Allocation memoire qui pose probleme (cudaRes) \n");
	}

	long dimBlock = blocksize;
	long dimGrid = tailleVecteur/blocksize;
	if ((tailleVecteur % blocksize) != 0) {
		dimGrid++;
	}

	int res = cudaMemcpy(&cudaVec[0], &vecteur[0], size, cudaMemcpyHostToDevice);


	
StartTimerGPU;
 	//Do kernel activity here
 	add_vec_scalaire_gpu<<<dimGrid, dimBlock>>>(cudaVec, cudaRes, alpha, tailleVecteur);
StopTimerGPU;
 
	cudaMemcpy(&resultat[0], &cudaRes[0], size, cudaMemcpyDeviceToHost);

PrintExecutionTimeGPU;



StartTimerCPU;

	add_vec_scalaire_cpu (vecteur, resultat, 10, tailleVecteur);

StopTimerCPU;

PrintExecutionTimeCPU;

}


