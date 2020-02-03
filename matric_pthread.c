#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
  
#define THREAD 3 
#define MAX 32 

pthread_t thread[3]; 

typedef struct matrix_size{
	int A_row;
	int A_col;
	int B_row;
	int B_col;
}matrix_size;

int A[MAX][MAX], B[MAX][MAX], Multiply[MAX][MAX]; 
int part = 0;
  
void *Multiplication(void *t) 
{ 
	matrix_size *m = (matrix_size *)t;
	int i, j, k, sum; 
    	int thread_part = part++; 
    
    	for (i = thread_part *  m->A_row / THREAD; i < (thread_part + 1) * m->A_row / THREAD; i++) { 
        	for (j = 0; j < m->A_col; j++) { 
        		sum = 0;
            		for(k = 0; k < m->B_row; k++)
            			sum += A[i][k]*B[k][j]; 
	    		Multiply[i][j] = sum;
        	} 
    	} 
  
} 
  
int main() 
{ 
	int i, j; 
	int m, n, p, q;
    	matrix_size *t = malloc(sizeof(matrix_size));
    	if(t == NULL)
		return -1;
	printf("input:\n");
    	scanf("%d%d", &m, &n);
    	for(i = 0; i < m; i++)
		for(j = 0; j < n; j++)
	   		scanf("%d", &A[i][j]);//A[i][j] = rand() % 10;
    	scanf("%d%d", &p, &q);
    	for(i = 0; i < p; i++)
		for(j = 0; j < q; j++)
	    		scanf("%d", &B[i][j]);//B[i][j] = rand() % 10; 
    	if(n != p){
		return -1;
    	}
    	t->A_row = m;
    	t->A_col = n;
    	t->B_row = p;
    	t->B_col = q;	
    	for (i = 0; i < THREAD; i++) { 
        	pthread_create(&thread[i], NULL, Multiplication, (void *)t); 
    	} 
    	for (i = 0; i < THREAD; i++) { 
        	pthread_join(thread[i], NULL); 
    	} 
        printf("output:\n%d %d\n", m, q);
    	for(i = 0; i < m; i++){
		for(j = 0; j < q; j++)
	    		printf("%d ", Multiply[i][j]); 
		printf("\n");
    	}
    	return 0; 
} 

