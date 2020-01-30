#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define BUILT 3
#define DELIM " \n\t\r\a"

#define MAX_CMDS 32
#define MAX_SIZE 128

#define read_end 0
#define write_end 1

#define BACKGROUND 0
#define FOREGROUND 1

typedef struct token{
	char **cmds;
	char *infile;
	char *outfile;
	int no_cmd;
	int mode;
}token;

char *BuiltinStr[] = {"cd", "kill", "quit"};

int cd(char **arg){ //cd
	if(!arg[1]){
		if(chdir("/home/hp") == -1){
			perror("cd failed");
			return -1;
		}
	}
	else{
		if(chdir(arg[1]) == -1){
			perror("cd failed");
			return -1;
		}
	}
	return 0;
}

int pkill(char **arg){ //kill
	int pid, signo;
	pid = atoi(arg[1]);
	signo = atoi(arg[2]);
	kill(pid, signo);
	return 0;
}

int quit(char **arg){ //exit
	exit(0);
}

int (*BuiltinFunc[]) (char **) = {&cd, &pkill, &quit};

token Process(char *str){
	token t;
	t.cmds = (char **)malloc(sizeof(char *) * MAX_CMDS);
	int i = 0, j, k, pause, no = 0, in = 0, out = 0, start;
	int n = strlen(str);
	pause = i;
	int rflag = 0;
	if(str[strlen(str) - 2] == '&'){
		t.mode = BACKGROUND; 
		str[strlen(str) - 2] = '\0';
	}
	else
		t.mode = FOREGROUND;
	for(i = 0; i < n; i++){
		if(str[i] == 60 || str[i] == 124 || str[i] == 62){
			rflag = 1;
			break;
		}
	}
	if(!rflag){ // simple command
		t.cmds[0] = str;
		t.no_cmd = 1;
		return t;
	}
	while(i < n){
		start = 1;
		switch(str[i]){
			case 60:{ // < infile
				if(!out){
					t.cmds[no] = (char *)malloc(sizeof(char) * MAX_SIZE); 
					for(j = pause, k = 0; j < i; j++, k++){
						t.cmds[no][k] = str[j];
					}
					t.cmds[no][k] = 0;
					++i, ++no;
					while(str[i] == 32)
						i += 1;
					pause = i;
				}
		
				t.infile = (char *)malloc(sizeof(char) * MAX_SIZE);
				for(j = 0, k = 0; i < n; j++){
					if(str[i] == 32  || str[i] == 60){
						if(start){
							i += 1;
							continue;
						}
						else
							break;
					}
					start = 0;		
					t.infile[k++] = str[i];
					i += 1;
				}
				t.infile[k] = 0;
				t.infile = strtok(t.infile, DELIM);
				in = 1;
				break;
			}
			case 62:{ // > outfile
				if(!in){
					t.cmds[no] = (char *)malloc(sizeof(char) * MAX_SIZE); 
					for(j = pause, k = 0; j < i; j++, k++){
						t.cmds[no][k] = str[j];
					}
					t.cmds[no][k] = 0;
					++i, ++no;
					while(str[i] == 32)
						i += 1;
					pause = i;
				}
			
				t.outfile = (char *)malloc(sizeof(char) * MAX_SIZE);
				for(j = 0, k = 0; i < n; j++){
					if(str[i] == 32 || str[i] == 62){
						if(start){
							i += 1;
							continue;
						}	
						else
							break;	
					}
					start = 0;
					t.outfile[k++] = str[i];
					i += 1;
				}
				t.outfile[k] = 0;
				t.outfile = strtok(t.outfile, DELIM);
				out = 1;
				break;
			}
			case 124: {// | pipe
				if(!in && !out){
					t.cmds[no] = (char *)malloc(sizeof(char) * MAX_SIZE); 
					for(j = pause, k = 0; j < i; j++, k++){
						t.cmds[no][k] = str[j];
					}
					t.cmds[no][k] = 0;
					i += 1; ++no;
					while(str[i] == 32)
						i += 1;
					pause = i;
					break;
				}
			}
		}
		i += 1;
	}
	if(!in && !out){ // | pipe + simple
		t.cmds[no] = (char *)malloc(sizeof(char) * MAX_SIZE); 
		for(j = pause, k = 0; j < i; j++, k++){
			t.cmds[no][k] = str[j];
		}
		t.cmds[no][k] = 0;
		++no;
	}
	t.no_cmd = no;
	return t;
		
}

int Parse(char *pros, char **args){
	char *t;
	char *str = strdup(pros);
	t = strtok(str, DELIM);
	int i = 0;
	while(t != NULL){
		args[i++] = t;
		t = strtok(NULL, DELIM);
	}
	args[i] = NULL;
	return 0;
}

int BuiltIn(token t){
	char **args = (char **)malloc(sizeof(char *) * MAX_CMDS);
	int i;
	Parse(t.cmds[0], args);
	if(args[0] == NULL)
		return -1;
	for (i = 0; i < BUILT; i++) {
    		if (!strcmp(args[0], BuiltinStr[i])){
			(*BuiltinFunc[i])(args);
    			return 0;
		}
    	}
	return -1;
}
int Execute(token t){
	char **args = (char **)malloc(sizeof(char *) * MAX_CMDS);
	int tmpin = dup(0);
	int tmpout = dup(1);
	int fdin;
	int pid;
	int i;
	
	if(t.no_cmd == 1 && !t.infile && !t.outfile){
		if(!BuiltIn(t)) // builtin function 
			return 0;
	}
	if(t.infile){
		fdin = open(t.infile, O_RDONLY);
		if(fdin == -1){
			perror("open failed");
			return -1;
		}
		
	}
	else{
		fdin = dup(tmpin);
		if(fdin == -1){
			perror("open failed");
			return -1;
		}
	}
	
	int fdout;
	for(i = 0; i < t.no_cmd; i++){
		dup2(fdin, 0);
		close(fdin);
		if(i == t.no_cmd - 1){
			if(t.outfile){
				fdout = open(t.outfile, O_WRONLY | O_CREAT, S_IWUSR);
				if(fdout == -1){
					perror("open failed");
					return -1;
				}
			}
			else{
				fdout = dup(tmpout);
				if(fdout == -1){
					perror("open failed");
					return -1;
				}
			}
		}
		else{
			int pfd[2];
			if(pipe(pfd) == -1){
				perror("pipe failed");
				return -1;
			}
			fdout = pfd[write_end];
			fdin = pfd[read_end];
		}
		dup2(fdout, 1);
		close(fdout);
		pid = fork();
		if(pid == -1){
			perror("fork failed");
			return -1;
		}
		if(pid == 0){
			Parse(t.cmds[i], args);
			if(args[0] == NULL){
				exit(0);
			}
			execvp(args[0], args);
			perror("execvp failed");
			exit(errno);
		}
		else if(t.mode){
			wait(NULL);
		}
	}
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	close(tmpin);
	close(tmpout);
	return 0;
}

void IntShell(){
	write(1, "---Practice Shell created by Shweta Suryavanshi.\n---Command supported by shell:\n---\"cd\"\n---\"quit\"\n---Multiple PIPE handling.\n---Input/Output redirection.\n---Space handling.\n---Background process.\n---All other general command supported by LINUX Shell.\n", 251);
}
void PrintCurrDir(){
	char cwd[MAX_SIZE];
	getcwd(cwd, MAX_SIZE);
	printf("\n%s\n", cwd);	
}

void SignalHandler(int n){
	write(1, "\nTo exit type \"quit\"\n", 21);
	PrintCurrDir();
	write(1, "prompt>>", 8);
}

int main(){
	signal(SIGINT, SignalHandler); //ctrl^c
	signal(SIGTSTP, SignalHandler);//ctrl^z
	char str[MAX_SIZE];
	IntShell();
	while(1){
		PrintCurrDir();
		write(1, "prompt>>", 8);
		fgets(str, MAX_SIZE, stdin);
		token t = Process(str);
		Execute(t);
	}
	return 0;
}
