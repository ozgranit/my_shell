#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h> // for wait macros etc
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#define _GNU_SOURCE
#define WRITE_END 1
#define READ_END 0

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
	int i,status;
	pid_t cpid1,cpid2;
	int fd[2];
	bool background = false;
	bool pipemode = false;
	
	//check for "|" in command
	for(i = 0; i < count; i++){
		if(strcmp(arglist[i],"|") == 0){
			pipemode = true;
			arglist[i] = NULL; // replace "|" with NULL
			//i now contains index of "|'
			break;
		}
	}
	
	if(pipemode){
	
		if(pipe(fd) < 0){
			perror(strerror(errno));
			exit(1);
		}
		
		cpid1 = fork();
		
		if(cpid1==0)
		{
			// define sigint handeling as default
			if(signal(SIGINT, SIG_DFL) == SIG_ERR){
				perror(strerror(errno));
				exit(1);
			}	
			
			dup2(fd[WRITE_END], STDOUT_FILENO);
			close(fd[READ_END]);
			close(fd[WRITE_END]);
			if(execvp(arglist[0],arglist) < 0){ //execvp will end read at null
				perror(strerror(errno));
				exit(1);
			}
		}
		else
		{ 
			//parent make second child
			cpid2=fork();

			if(cpid2==0)
			{
				// define sigint handeling as default
				if(signal(SIGINT, SIG_DFL) == SIG_ERR){
					perror(strerror(errno));
					exit(1);
				}
				
				dup2(fd[READ_END], STDIN_FILENO);
				close(fd[WRITE_END]);
				close(fd[READ_END]);
				//execvp second command
				if(execvp(arglist[i+1],&arglist[i+1]) < 0){ 
					perror(strerror(errno));
					exit(1);
				}
			}
			else
			{
				close(fd[READ_END]);
				close(fd[WRITE_END]);
				//wait for both children to finish
				//dont wait for all backgraound proccess to finish
				waitpid(cpid1,&status,0);
				waitpid(cpid2,&status,0);
			}
		}
	}		
	//no pipe in command
	else{
		if(strcmp(arglist[count-1],"&") == 0){
			arglist[count-1] = NULL; // replace "&" with NULL
			count--;
			background = true;
		}
		
		
		cpid1 = fork();
		
		if(cpid1 == 0){
			//bg proccess dont die on ^c
			if(!background){
				// define sigint handeling as default
				if(signal(SIGINT, SIG_DFL) == SIG_ERR){
					perror(strerror(errno));
					exit(1);
				}
			}
					
			if(execvp(arglist[0],arglist) < 0){
				perror(strerror(errno));
				exit(1);
			}
			//unreachable code
			assert(false);
		}
	
	
		if(!background){
			//wait for one child to finish
			//dont wait for all backgraound proccess to finish
			waitpid(cpid1,&status,0);
		}
		else{
			//dont create a zombie when background proc is done
			if(signal(SIGCHLD,SIG_IGN) == SIG_ERR){
				perror(strerror(errno));
				return 0;
			}
		}
	}
	//success
	return 1;
}

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
	// define sigint handeling as do nothing
	if(signal(SIGINT, SIG_IGN) == SIG_ERR){
		perror(strerror(errno));
		return -1;
	}
	//success
	return 0;
}
int finalize(void){
	// define sigint handeling as default
	if(signal(SIGINT, SIG_DFL) == SIG_ERR){
		perror(strerror(errno));
		return -1;
	}
	//success
	return 0;
}

