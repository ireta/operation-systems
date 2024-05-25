#include "hw2_output.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


int **A, **B, **C, **D, **J, **L, **R;
unsigned row1, col1, row2, col2, row3, col3, row4, col4;

pthread_t *ptid_J, *ptid_L, *ptid_R;

sem_t *sem_J, **sem_L;

void* calculate_J(void *row_par){
	unsigned row = *(unsigned *)(&row_par);
	unsigned i;
	for(i=0 ; i<col1 ; ++i){
		J[row][i] = A[row][i] + B[row][i];
		hw2_write_output(0, row+1, i+1, J[row][i]);
	}
	sem_post(&sem_J[row]);
}

void* calculate_L(void *row_par){
	unsigned row = *(unsigned *)(&row_par);
	unsigned i;
	for(i=0 ; i<col3 ; ++i){
		L[row][i] = C[row][i] + D[row][i];
		hw2_write_output(1, row+1, i+1, L[row][i]);
		sem_post(&sem_L[row][i]);
	}
}

void* calculate_R(void *row_par){
	unsigned row = *(unsigned *)(&row_par);
	unsigned i, j;
	sem_wait(&sem_J[row]);
	for(i=0 ; i<col3 ; ++i){
		for(j=0 ; j<row3 ; ++j){
			sem_wait(&sem_L[j][i]);
			R[row][i] += J[row][j] * L[j][i];
			sem_post(&sem_L[j][i]);
		}
		hw2_write_output(2, row+1, i+1, R[row][i]);
	}
	sem_post(&sem_J[row]);
}

void print_R(){
	unsigned i, j;
	for(i=0 ; i<row1 ; ++i){
		for(j=0 ; j<col3 ; ++j){
			printf("%d ", R[i][j]);
		}
		printf("\n");
	}
}

void read_inputs(){
	
	unsigned i, j;
	scanf("%d %d ", &row1, &col1);
	A = (int**)malloc(row1*sizeof(int*)); 
	J = (int**)malloc(row1*sizeof(int*));
	ptid_J = (pthread_t*)malloc(row1*sizeof(pthread_t));
	ptid_L = (pthread_t*)malloc(col1*sizeof(pthread_t));
	ptid_R = (pthread_t*)malloc(row1*sizeof(pthread_t));
	sem_J = (sem_t*)malloc(row1*sizeof(sem_t));
	for(i=0 ; i<row1 ; ++i){
		A[i] = (int*)malloc(col1*sizeof(int));
		J[i] = (int*)malloc(col1*sizeof(int));
		sem_init(&sem_J[i], 1, 0);
		for(j=0 ; j<col1 ; ++j){
			scanf("%d ", &A[i][j]);
			J[i][j] = 0;
		}
	}
	
	scanf("%d %d ", &row2, &col2);
	B = (int**)malloc(row2*sizeof(int*)); 
	for(i=0 ; i<row2 ; ++i){
		B[i] = (int*)malloc(col2*sizeof(int));
		for(j=0 ; j<col2 ; ++j){
			scanf("%d ", &B[i][j]);
		}
	}
	
	scanf("%d %d ", &row3, &col3);
	C = (int**)malloc(row3*sizeof(int*));
	L = (int**)malloc(row3*sizeof(int*));
	sem_L = (sem_t**)malloc(row3*sizeof(sem_t*));
	for(i=0 ; i<row3 ; ++i){
		C[i] = (int*)malloc(col3*sizeof(int));
		L[i] = (int*)malloc(col3*sizeof(int));
		sem_L[i] = (sem_t*)malloc(col3*sizeof(sem_t));
		for(j=0 ; j<col3 ; ++j){
			scanf("%d ", &C[i][j]);
			L[i][j] = 0;
			sem_init(&sem_L[i][j], 1, 0);
		}
	}
	
	scanf("%d %d ", &row4, &col4);
	D = (int**)malloc(row4*sizeof(int*)); 
	for(i=0 ; i<row4 ; ++i){
		D[i] = (int*)malloc(col4*sizeof(int));
		for(j=0 ; j<col4 ; ++j){
			scanf("%d ", &D[i][j]);
		}
	}
	
	R = (int**)malloc(row1*sizeof(int*));
	for(i=0 ; i<row1 ; ++i){
		R[i] = (int*)malloc(col3*sizeof(int));
		for(j=0 ; j<col3 ; ++j){
			R[i][j] = 0;
		}
	}
}

void free_matrices(){

	unsigned i, j;
	for(i=0 ; i<row1 ; ++i){
		free(A[i]);
		free(B[i]);
		free(J[i]);
		free(R[i]);
	}
	for(i=0 ; i<row3 ; ++i){
		free(C[i]);
		free(D[i]);
		free(L[i]);
		free(sem_L[i]);
	}
	free(sem_J);
	free(ptid_J);
	free(ptid_L);
	free(ptid_R);
}

int main()
{
	hw2_init_output();
	unsigned long i, j;

	read_inputs();
	
	for(i=0 ; i<row1 ; ++i){
		pthread_create(&ptid_J[i], NULL, &calculate_J, (void *)i);
	}
	for(i=0 ; i<col1 ; ++i){
		pthread_create(&ptid_L[i], NULL, &calculate_L, (void *)i);
	}
	for(i=0 ; i<row1 ; ++i){
		pthread_create(&ptid_R[i], NULL, &calculate_R, (void *)i);
	}
	
	for(i=0 ; i<row1 ; ++i){
		pthread_join(ptid_J[i], NULL);
	}
	
	for(i=0 ; i<col1 ; ++i){
		pthread_join(ptid_L[i], NULL);
	}
	for(i=0 ; i<row1 ; ++i){
		pthread_join(ptid_R[i], NULL);
	}
	//printf("%d %d", row1, col3);
	print_R();
	
	free_matrices();
	return 0;
}
