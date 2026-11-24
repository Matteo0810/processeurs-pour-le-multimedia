#include <stdio.h>
#include <sys/time.h>    


#include "../../MesuresTpsExec/CPUExecMeasurements.h"
#include "../../MesuresTpsExec/GPUExecMeasurements.h"


long tailleVecteur ;

/* KERNEL CUDA */

__global__ void add_vec_scalaire_gpu(int *vec, int *res, int a, long N) {
	long i = (long)blockIdx.x * (long)blockDim.x + (long)threadIdx.x;
	if (i < N) {
		res[i] = vec[i] + a;
	}
}

void add_vec_scalaire_cpu(int *vec, int *res, int a, long N) 
{
	int i ;
	for (i=0 ; i < N ; i ++) {
		res[i] = vec[i] + a;
	}   
	
}

int main(int argc, char *argv[]) {
	int alpha = 10;
	if (argc < 2) {
		printf("Erreur, manque un argument\n");
		exit(0);
	}
	tailleVecteur = atol(argv[1]);
	long blocksize = 1;	
	if (argc ==3) {
		blocksize = atoi(argv[2]);
	}

	int *vecteur;
	int *resultatGPU;
	int *resultatCPU;
	int *cudaVec;
	int *cudaRes;

InitGPUTimer;
InitCPUTimer;

	long size = sizeof(int)*tailleVecteur;

	vecteur = (int *)malloc(size);
	resultatCPU = (int *)malloc(size);
	resultatGPU = (int *)malloc(size);
	
	if (vecteur == NULL) {
		printf("Allocation memoire pour CPU qui pose probleme (vecteur) \n");
	}
	if (resultatCPU == NULL) {
		printf("Allocation memoire pour CPU qui pose probleme (resultat) \n");
	}

	if (resultatGPU == NULL) {
		printf("Allocation memoire pour GPU ui pose probleme (resultat) \n");
	}

	long i ;
	for (i= 0 ; i < tailleVecteur ; i++) {
		vecteur[i] = rand() % 100;
		resultatCPU[i] = 0;
		resultatGPU[i] = 0;
	}

/*	cudaSetDevice(1);	*/

	if (cudaMalloc((void **)&cudaVec, size) == cudaErrorMemoryAllocation) {
		printf("Allocation memoire qui pose probleme (cudaVec) \n");
	}
	if (cudaMalloc((void **)&cudaRes, size)  == cudaErrorMemoryAllocation) {
		printf("Allocation memoire qui pose probleme (cudaRes) \n");
	}

	dim3 DimBlock(blocksize,1,1);
	
	long nbBlocks = tailleVecteur/blocksize ;
	if ((tailleVecteur % blocksize) != 0) {
		nbBlocks++;
	}
	dim3 DimGrid(nbBlocks, 1, 1);


	printf("------ ");
	printf("dimGrid %ld,1,1 dimBlock %ld,1,1 \n",nbBlocks, blocksize);



	int res = cudaMemcpy(&cudaVec[0], &vecteur[0], size, cudaMemcpyHostToDevice);

	printf("Copy CPU -> GPU %d \n",res);
StartGPUTimer;
	add_vec_scalaire_gpu<<<DimGrid, DimBlock>>>(cudaVec, cudaRes, alpha, tailleVecteur);
StopGPUTimer;



	cudaMemcpy(&resultatGPU[0], &cudaRes[0], size, cudaMemcpyDeviceToHost);


	/* Test bon fonctionnement */



StartCPUTimer;
	add_vec_scalaire_cpu (vecteur, resultatCPU, alpha, tailleVecteur);
StopCPUTimer;


	bool ok = true;
	int indice = -1;
	for (i= 0 ; i < tailleVecteur ; i++) {
/*		printf("Resultat GPU %d     Resultat CPU %d \n",resultat[i], vecteur[i]+alpha);	*/
		if (resultatGPU[i] != resultatCPU[i] ) {
			ok = false;
			if (indice ==-1) {
				indice = i;
			}
		}
	}

	if (ok) {
		printf("Verification Resultats CPU == GPU : ok\n");
	} else {
		printf("Verification Resultats CPU == GPU : PROBLEME ! (%d)\n", indice);
	}


PrintGPUExecutionTime("Temps GPU ", GPUExecTps);

PrintCPUExecutionTime("Temps CPU ", CPUSecExecTps);
PrintCPUExecutionTime("Temps CPU ", CPUMilliSecExecTps);
PrintCPUExecutionTime("Temps CPU ", CPUMicroSecExecTps);
PrintCPUExecutionTime("Temps CPU ", CPUNanoSecExecTps);


	cudaFree(cudaVec);
	cudaFree(cudaRes);


}

