#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX 64
/*
 * I didn't understand what exactly need to do here
 * I'm just doing copying file from input to copy
*/

void random_copy(int fd){
	int copy = open("copy", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if(copy == -1){
		perror("Open failed");
		exit(errno);
	}
	
	int length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	unsigned int used[10] = {0};
	int count = 0;
	int chunk_size;
	if(!(length % 10))
		chunk_size = length % 10;
	else
		chunk_size = length / 10 + 1;
	int offset, i, n;
	char buff[MAX];
	printf("length %d\n", length);
	printf("chunk_size %d\n", chunk_size);
	for(i = 0; count < 10; i++){
		offset = rand() % 10;
		if(! used[offset]){
			lseek(fd, offset * chunk_size, SEEK_SET);
			n = read(fd, buff, chunk_size);
			if(lseek(copy, offset * chunk_size, SEEK_SET) == -1){
				printf("something you missing\n");
				exit(0);
			}
			write(copy, buff, n);
			count += 1;
			used[offset] = 1;
		}
	}
	close(copy);
}

int main(int argc, char *argv[]){
	if(argc < 1){
		perror("less arguments");
		exit(-1);
	}
	int fd = open(argv[1], O_RDONLY);
	if(fd == -1){
		perror("file not found");
		exit(errno);
	}
	random_copy(fd);
	close(fd);
	return 0;
}
