#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
//#include "ext2_fs.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#define DELIM "/\n\t"

void read_fs(char *device_file, char *str, char *arg){
	int fd = open(device_file, O_RDONLY); // argv[1] = /dev/sdb1 
	if(fd == -1) {
		perror("readsuper:");
		exit(errno);
	}
	struct ext2_super_block sb; 
	struct ext2_group_desc bgdesc;
	struct ext2_inode inode;
	struct ext2_dir_entry_2 dirent;	
	
	unsigned int block_size;
	unsigned int inode_size;
	unsigned int block_group;
	unsigned int index;
	int count;

	lseek(fd, 1024, SEEK_CUR);
	count = read(fd, &sb, sizeof(struct ext2_super_block)); //super_block


	block_size = 1024 << sb.s_log_block_size; //block_size
	inode_size = 1 << sb.s_log_block_size;

	lseek(fd, block_size, SEEK_SET);
	count = read(fd, &bgdesc, sizeof(struct ext2_group_desc)); //block group descriptor

	lseek(fd, bgdesc.bg_inode_table * block_size + 2 * sizeof(struct ext2_inode), SEEK_SET);
	read(fd, &inode, sizeof(struct ext2_inode)); //read root inode
	
	char *t = strtok(str, DELIM);
	while(t != NULL){
		lseek(fd, inode.i_block[0] * block_size, SEEK_SET);
		
		while(1){
			read(fd, &dirent, sizeof(struct ext2_dir_entry_2)); //read dirent
			
			/*
			 * dirent.inode == 0
			 * then that entry doesn't exist
			 */
			if(!dirent.inode){
				printf("Directory/File not found\n");
				return ;
			}
			/*
			 * compare dirent.name and t
			 * if same, jump to it's inode and read inode
			 * else read next entry from block
			 */
			dirent.name[dirent.name_len] = '\0';
			if(!strcmp(dirent.name, t)){ 
				block_group = (dirent.inode - 1) / sb.s_inodes_per_group; //block_group of inode
				index = (dirent.inode - 1) % sb.s_inodes_per_group; //index of inode

				lseek(fd, block_group * sizeof(struct ext2_group_desc) + block_size, SEEK_SET); 
				read(fd, &bgdesc, sizeof(struct ext2_group_desc)); // read block_group_descriptor
				
				lseek(fd, bgdesc.bg_inode_table * block_size + index * sizeof(struct ext2_inode), SEEK_SET);
				read(fd, &inode, sizeof(struct ext2_inode)); //read inode
				break;
			}

			lseek(fd, dirent.rec_len - sizeof(struct ext2_dir_entry_2), SEEK_CUR);
		}
		t = strtok(NULL, DELIM);	
	}
	
	char buff[block_size];
	char c;
	int i;
	if(!strcmp(arg, "inode")){ // 2nd argument checking
		printf("Inode no: %u\n", dirent.inode);
	}
	else{// print data
		lseek(fd, inode.i_block[0] * block_size, SEEK_SET);
		
		if(inode.i_mode == EXT2_S_IFDIR){ // inode represent directory

			while(1){
				read(fd, &dirent, sizeof(struct ext2_dir_entry_2)); //read dirent
				if(!dirent.inode)
					break;
				dirent.name[dirent.name_len] = '\0';
				printf("%s\n", dirent.name);
				lseek(fd, dirent.rec_len - sizeof(struct ext2_dir_entry_2), SEEK_CUR);				
			}
			 
		}
		else{ // inode represent text file
			
				read(fd, buff, block_size);
				for(i = 0; buff[i] != EOF; i++) {
					printf("%c", buff[i]);
				}
				
				
			
		}
	}
	
}

int main (int argc, char *argv[]){
	if(argc < 4){
		perror("Less arguments:");
		exit(1);
	}
	char *t = strtok(argv[3], DELIM);
	if(!strcmp(t, "inode") || !strcmp(t, "data")){
		read_fs(argv[1], argv[2], t);
	}
	else{
		printf("Incorrect argument\n");
	}
	return 0;
}

