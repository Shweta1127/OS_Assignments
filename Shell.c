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
#define DELIM_OP " <|>"

#define MAX_CMDS 32
#define MAX_SIZE 128

#define read_end 0
#define write_end 1

#define BACKGROUND 0
#define FOREGROUND 1

#define INPUT 0
#define OUTPUT 1

int *BackGround;
int top = 0;
typedef struct token{
	char **cmds;
	char *infile;
	char *outfile;
	int no_cmd;
	int mode;
}token;

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

int quit(char **arg){ //exit
	printf("Bye!\n");
	exit(0);
}

int bg(char **arg){
	int pid, i;
	int index = atoi(arg[1]);
	if(!index){
		return -1;
	}
	pid = BackGround[index - 1];
	if(kill(pid, SIGCONT) < 0){
		perror("kill failed");
		return -1;
	}
	top -= 1;
	for(i = index - 1; i < top; i++)
		BackGround[i] = BackGround[i + 1];
	return 0;
}

int InsertCMD(token *t, char *str, int pause, int stop){
	int i, j;
	t->cmds[t->no_cmd] = (char *)malloc(sizeof(char) * MAX_SIZE); 
	for(i = pause, j = 0; i < stop; i++, j++){
		t->cmds[t->no_cmd][j] = str[i];
	}
	t->cmds[t->no_cmd][j] = 0;
	++t->no_cmd;
	return 0;
}

int File(token *t, char *str, int start, int end, int type){
	char *file = (char *)malloc(sizeof(char) * MAX_SIZE);
	int k = 0;
	int flag = 1;
	while(start < end){
		if(str[start] == 32  || str[start] == 60 || str[start] == 62){
			if(flag){
				start += 1;
				continue;
			}
			else
				break;
		}
		flag = 0;
		file[k++] = str[start];
		start += 1;
	}
	file[k] = 0;
	file = strtok(file, DELIM);
	if(!type)
		t->infile = file;
	else
		t->outfile = file;
	return start;
}

token Process(char *str){
	token t;
	t.cmds = (char **)malloc(sizeof(char *) * MAX_CMDS);
	int i = 0, pause;
	int in = 0, out = 0;
	int n = strlen(str);
	int rflag = 0;

	t.no_cmd = 0;
	pause = i;

	char *dummy = strdup(str);
	char *tok = strtok(dummy, DELIM_OP);
	tok = strtok(NULL, DELIM_OP);
	if(tok != NULL){
		rflag = 1;
	}
	if(!rflag){ // simple command
		t.cmds[0] = str;
		t.no_cmd = 1;
		return t;
	}

	while(i < n){
		switch(str[i]){
			case '<':{ // < infile
				if(!out){
					InsertCMD(&t, str, pause, i);
					i++;
					while(str[i] == 32)
						i += 1;
					pause = i;
				}
				else{
					i++;				
				}
				i = File(&t, str, i, n, INPUT);
				in = 1;
				break;
			}
			case '>':{ // > outfile
				if(!in){
					InsertCMD(&t, str, pause, i);
					i++;
					while(str[i] == 32)
						i += 1;
					pause = i;
				}
				else{
					i++;
				}
				i = File(&t, str, i, n, OUTPUT);
				out = 1;
				break;
			}
			case '|': {// | pipe
				if(!in && !out){
					InsertCMD(&t, str, pause, i);
					i++;
					while(str[i] == 32)
						i += 1;
					pause = i;
					break;
				}
			}
		}
		i += 1;
	}
	if(!in && !out){
		InsertCMD(&t, str, pause, i);
	}
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
	if(!strcmp(args[0], "cd")){
		cd(args);
		return 0;
	}
	if(!strcmp(args[0], "bg")){
		bg(args);
		return 0;
	}
	if(!strcmp(args[0], "quit")){
		quit(args);
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
	int status;
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
				fdout = open(t.outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
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
			do{
				/*code to detect child is suspended;
				 * adding it's pid to BackGround; 
				 * stop waiting for it to terminate
				 */
				waitpid(pid, &status, WUNTRACED | WCONTINUED);
				if(WIFSTOPPED(status)){
					BackGround[top++] = pid;
					printf("[%d]+ stopped  %s\n", top, t.cmds[i]);
					break;
				}
			}while(!WIFEXITED(status) && !WIFSIGNALED(status));
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
	write(1, "\n", 1);
}

int main(){
	
	signal(SIGINT, SignalHandler); //ctrl^c
	signal(SIGTSTP, SignalHandler);//ctrl^z
	BackGround = (int *)malloc(sizeof(char) * MAX_CMDS);
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
