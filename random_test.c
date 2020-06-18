#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX 64

int main(int argc, char *argv){
	int i;
	unsigned int used[10] = {0};
	int count = 0;
	int j;
	for(i = 0; count < 10; i++){
		j = rand() % 10;
		if(! used[j]){
			printf("%d\n", j);
			used[j] = 1;
			count += 1;
		}
	}
	printf("%d\n", i);
	return 0;
}

