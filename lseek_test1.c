#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX 64

int main(int argc, char *argv[]){
	if(argc < 4){
		perror("less arguments");
		exit(-1);
	}
	char *path = argv[1];
	int offset = atoi(argv[2]);
	int len = atoi(argv[3]);

	int fd = open(path, O_RDONLY);
	if(fd == -1){
		perror("file not found");
		exit(errno);
	}

	char *buff = (char *)malloc(MAX);
	lseek(fd, offset, SEEK_SET);
	int n = read(fd, buff, len);

	if(!strcmp(buff, argv[4])){
		printf("same\n");
	}
	else{
		printf("not same\n");
	}
	close(fd);

	return 0;
}

