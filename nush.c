// Templates of the functions were taken from Nat Tuck's lecture notes 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "tokenize.h"
#include "svec.h"
#include <fcntl.h>

// Provided files (hints folder) were modifed, and their functionality was extended
// (svec_contains(), split_svec(), split2_svec() , etc. )

// Function to check the error
void check(int n) {
	if (n == -1) {
		perror("error");
		exit(1);}
}

// Function to execute tokenized input
// Function has 4 arguments:
// 1. Svec* sv : array of tokens
// 2. int setting : is one of (1(start),11 (middle processes),111(end) - pipe , 2 - OR/AND, 3 - &, 4,5 - parentheses)
// 3. int buf : input buffer
// 4. int buf2 : output buffer

//Structure of the function was approved by the professor ("no need to split into smaller functions")

int execute(svec* sv, int setting, int buf, int buf2)
{
	// first, execute() function checks presence of the operators (ordered by significance)
	// it splits svec into array of svec when operator is found and executes each part recursively
	// implementation of execution varies depending on the operator
	// two pipes (for two-way conntection) and fork are used to execute functions parallelly with main shell process
 
	// Array of svecs  
	svec** ssv;
	// Template to store a string
	char tmp[1000];
	// Child pid
	int cpid;
	// Return value
	int rv;
	// File descriptor
	int fd;
	// Size of the array of svecs
	int size;
	// Counter
	int ii;
	// Two pipes for input/output connection
	int pipes[2];  // input pipe
	int pipes2[2]; // output pipe 
	// Check if the command is exit
	if ((sv->size == 1)&&(strcmp(sv->data[0], "exit")==0)){
		exit(0);
	}
	// Check if the command is cd
	if ((sv->size == 2) && (strcmp(sv->data[0], "cd")==0)){
		chdir(sv->data[1]);
		return 1;
	}   
	// Check if open parentheses 
	// (execute expression in parentheses and redirect output to the next command)
	if ((setting == 4) && (svec_contains(sv, ")"))) { 
		ssv = split2_svec(sv, ")");  
		execute(ssv[0], 5, 0, 0);
		execute(ssv[1], 111, 0, 0);
		free_svec(ssv[0]);
		free_svec(ssv[1]);  
		free (ssv);
		return 1;
	}
	// Check if close parentheses 
	if (svec_contains(sv, "(")) { 
		ssv = split2_svec(sv, "(");  
		execute(ssv[1], 4, 0, 0);
		execute(ssv[0], 0, 0, 0);
		free_svec(ssv[0]);
		free_svec(ssv[1]);  
		free (ssv);
		return 1;
	}
	// Check if semicolon
	if (svec_contains(sv,";")) { 
		ssv = split_svec(sv, ";"); 
		for (ii = 0; ii<10;ii++){
			if (ssv[ii] !=0) {execute(ssv[ii], 0, 0, 0); free_svec(ssv[ii]);}
		}
		free(ssv);
		return 0;
	}
	// Check if & sign 
	if (svec_contains(sv,"&")) { 
		ssv = split2_svec(sv, "&"); 
		execute(ssv[1],0,0,0);
		execute(ssv[0],3,0,0);
		free_svec(ssv[0]);
		free_svec(ssv[1]);
		free(ssv);
		return 0;
	}
	// Check if >
	if (svec_contains(sv,">")) { 
		ssv = split2_svec(sv, ">"); 
		fd = open(ssv[1]->data[0], O_CREAT | O_TRUNC | O_RDWR, 0644); 
		execute(ssv[0], 0, 0, fd);
		free_svec(ssv[0]);
		free_svec(ssv[1]);
		free (ssv);
		return 0;
	}
	// Check if pipe
	if (svec_contains(sv,"|")) { 
		ssv = split_svec(sv, "|");
		size = 0;
		for (ii = 0; ii<10;ii++){
			if (ssv[ii] !=0) size++;
		}  
		// case of the first element of the pipe
		execute(ssv[0], 1, buf, 0);
		for (ii = 1; ii<size-1;ii++){
			// case of the middle elements of the pipe
			execute(ssv[ii], 11, 0, 0);
		}       
		// case of last element of the pipe
		execute(ssv[size-1], 111, 0, buf2);

		for (ii = 0; ii<size;ii++){
			free_svec(ssv[ii]);
		}  
		free(ssv);
		return 0;
	}
	// Check if contains AND
	if (svec_contains(sv, "&&")) { 
		ssv = split_svec(sv, "&&"); 
		size = 0;
		for (ii = 0; ii<10;ii++){
			if (ssv[ii] !=0) size++;
		}  
		for (ii = 0; ii<size;ii++){
			if ((strcmp(ssv[ii]->data[0], "false")==0) || !(execute(ssv[ii],2,0,0))) break;
		}  
		for (ii = 0; ii<size;ii++){
			free_svec(ssv[ii]);
		}  
		free (ssv);
		return 0;
	}
	// Check if contains OR
	if (svec_contains(sv, "||")) { 
		ssv = split_svec(sv, "||"); 
		size = 0;
		for (ii = 0; ii<10;ii++){
			if (ssv[ii] !=0) size++;
		}  
		for (ii = 0; ii<size;ii++){
			if ((strcmp(ssv[ii]->data[0], "false")==0) || !(execute(ssv[ii],2,0,0))) continue;
			break;
		}  
		for (ii = 0; ii<size;ii++){
			free_svec(ssv[ii]);
		}  
		free (ssv);
		return 0;
	}
	// Check if contains <
	if (svec_contains(sv,"<")) { 
		ssv = split_svec(sv, "<"); 
		fd = open(ssv[1]->data[0], O_RDONLY);
		execute(ssv[0], setting, fd, buf2);
		free_svec(ssv[0]);
		free_svec(ssv[1]);
		free (ssv);
		return 0;
	}
	// Pipe
	rv=pipe(pipes);
	check(rv);
	rv=pipe(pipes2);
	check(rv);
	// Fork
	if ((cpid = fork())) {
		// Parent process
		// (all setting values are described above)
		int status;
		if (setting == 1) {
			close(pipes[1]);
			waitpid(cpid, &status, 0);
			for (ii =0; ii < 1000; ii++) tmp[ii] = 0;
			rv=read(pipes[0], tmp, 1000); 
			check(rv);
			close(pipes[0]);  
			return 0; 
		} 
		if (setting == 111) {
			close(pipes[0]);
			rv=write(pipes[1], tmp, strlen(tmp));
			check(rv);
			close(pipes[1]);
			waitpid(cpid, &status, 0);
			return 0;
		}
		if ((setting==5) || (setting == 11)) {
			close(pipes[0]);
			close(pipes2[1]);
			rv=write(pipes[1], tmp, strlen(tmp));
			check(rv);
			close(pipes[1]);
			for (ii =0; ii < 1000; ii++) tmp[ii] = 0;
			rv=read(pipes2[0], tmp, 1000); 
			check(rv);
			close(pipes2[0]); 
			return 0;
		}
		if (setting == 2) {
			close(pipes[1]);
			waitpid(cpid, &status, 0);
			for (ii =0; ii < 1000; ii++) tmp[ii] = 0;
			rv=read(pipes[0], tmp, 1000); 
			check(rv);
			close(pipes[0]);  
			if (strcmp(tmp, "\0") == 0) return 1;
			execute(sv,0,0,0);
			return 0;
		}
		if(setting == 3) return 0;
		waitpid(cpid, &status, 0);
		return 0;
		//Executed program complete
		//Child returned
		if (WIFEXITED(status)) {
			printf("child exited with exit code (or main returned) %d\n", WEXITSTATUS(status));
		}
	}
	else {
		// Child process
		// all setting values are described above)
		if (setting == 1){ 
			close(pipes[0]);
			rv=dup2(pipes[1],1);
			check(rv);
			close(pipes[1]);
		}
		if (setting==11) {
			close(pipes[1]);
			close(pipes2[0]);
			rv=dup2(pipes[0],0);
			check(rv);
			rv=dup2(pipes2[1],1);
			check(rv);
			close(pipes[0]);
			close(pipes2[1]);
		}
		if (setting==111) {
			close(pipes[1]);
			rv=dup2(pipes[0],0);
			check(rv);
			close(pipes[0]);
		}      
		if (setting == 2){ 
			close(pipes[0]);
			rv=dup2(pipes[1],1);
			check(rv);
			close(pipes[1]);
		}
		if (setting ==5){
			close(pipes[1]);
			close(pipes2[0]);
			execute(sv, 0, pipes[0], pipes2[1]);
			close(pipes[0]);
			close(pipes2[1]);
		}
		if (buf!=0) {
			rv=dup2(buf, 0);
			check(rv);
		}
		if (buf2!=0) {
			rv=dup2(buf2, 1);
			check(rv);
		}
		rv=execvp(sv->data[0], sv->data);
		check(rv);
		printf("Can't get here, exec only returns on error.\n");
	}
}

// Main function that starts the nush program and tokenize the input
int main(int argc, char* argv[])
{
	char cmd[256];
	while (1){
		// if one argument
		if (argc == 1) {
			printf("nush$ ");
			fflush(stdout);
			if (!fgets(cmd, 256, stdin)){
				printf("\n");
				break;
			}
			svec* sv = tokenize(cmd);
			execute(sv,0,0,0);
			free_svec(sv);
		}
		// if more than one argument
		else {
			int ii;
			svec* sv = make_svec();
			for (ii=1; ii<argc; ii++) svec_push_back(sv, argv[ii]);
			execute(sv,0,0,0);
			free_svec(sv);
			break;
		}
	}
	// exit shell 
	return 0;
}