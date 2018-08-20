/****************************************************************
 * Name        : Jianfei Zhao                                   *
 * Class       : CSC 415                                        *
 * Date        : 07/06/2018                                     *
 * Description :  Writting a simple bash shell program          *
 * 	          that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/
//May add more includes
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <ctype.h>
//Max amount allowed to read from input
#define BUFFERSIZE 256
//Shell prompt
#define PROMPT "myShell"
//sizeof shell prompt
#define PROMPTSIZE sizeof(PROMPT)



// 0 for normal exit
// some possible errors
#define NORMAL_EXIT 0
#define FORK_ERR 1
#define PARAM_MISSING_ERR 2
#define PIPE_ERR 3
#define CHILD_PROCESS_ERR 4

char* getCurrDir(char* buffer);
char* checkDir(char* path);
int parse(char** myargv, char* buffer, char* delim);
int execCmd(char** myargv, int myargc);
int execPipe(char** myargv, int left, int right);
int execRedir(char** myargv, int left, int right);

int main(int* argc, char** argv) {
	char* myargv[32] = {NULL};
	char* delim = " \t\r\n";
	int myargc;
	char buffer[BUFFERSIZE];
	char* currDir;

	while (1) {
		currDir = getCurrDir(buffer);
		printf("%s%s >> ", PROMPT, currDir);
		fgets(buffer, BUFFERSIZE, stdin);
		myargc = parse(myargv, buffer, delim);

		if (myargv[0] == NULL) {
			continue;
		} else if (strcmp(myargv[0], "exit") == 0) {
			exit(0);
		} else if (strcmp(myargv[0], "cd") == 0) {
			chdir(checkDir(myargv[1]));
			continue;
		}

		execCmd(myargv, myargc);
	}

	return 0;
}



// retrieve the current working directory
// "/home/username" replaced by "~"
char* getCurrDir(char* buffer) {
	char* tmp;
	char currDir[1024] = "~";
	getcwd(buffer, BUFFERSIZE);
	char* home = getenv("HOME");
	if (home != NULL && (tmp = strstr(buffer, home)) != NULL) {
		return strcat(currDir, tmp + strlen(home));
	} else {
		return buffer;
	}
}

// check if path is empty or begins with "~"
// replace the front "~" by "home/username" if possible
// path is used for chdir()
char* checkDir(char* path) {
	char* home = getenv("HOME");
	if (path == NULL) {
		path = home;
	} else if (path[0] == '~') {
		char tmp[strlen(path) + strlen(home) + 10];
		int i = 0, j = 1;
		while (home[i] != '\0') {
			tmp[i] = home[i];
			i++;
		}
		while (path[j] != '\0') {
			tmp[i++] = path[j++];
		}
		tmp[i] = '\0';
		path = tmp;
	}
	return path;
}

// parse the command
// delim: delimiters
// myargv: command tokens
int parse(char** myargv, char* buffer, char* delim) {
	int i = 0;
	if ((myargv[i] = strtok(buffer, delim)) != NULL) {
		while ((myargv[++i] = strtok(NULL, delim)) != NULL) {}
	}
	return i;
}

// execute the command
// myargv: command tokens
// myargc: number of tokens
int execCmd(char** myargv, int myargc) {
	int result = NORMAL_EXIT;
	pid_t pid = fork();
	if (pid < 0) {
		return FORK_ERR;
	} else if (pid == 0) {
		// retrieve the file descriptors of stdin and stdout
		int fd_stdin = dup(STDIN_FILENO);
		int fd_stdout = dup(STDOUT_FILENO);

		// consider pipe
		// single command regarded as the basic element of pipe
		result = execPipe(myargv, 0, myargc);

		// restore the file descriptors of stdin and stdout
		dup2(fd_stdin, STDIN_FILENO);
		dup2(fd_stdout, STDOUT_FILENO);
		exit(result);
	} else {
		int status;
		waitpid(pid, &status, 0);
		if (!WIFEXITED(status)) {
			result = CHILD_PROCESS_ERR;
		}
		return result;
	}
}

// index range for command tokens: [left, right)
// pipe possibly included
int execPipe(char** myargv, int left, int right) { 
	if (left >= right) {
		return NORMAL_EXIT;
	}
	// check if pipe exists
	int pipeIndex = -1;
	for (int i = left; i < right; ++i) {
		if (strcmp(myargv[i], "|") == 0) {
			pipeIndex = i;
			break;
		}
	}

	// pipe does not exist
	if (pipeIndex == -1) {
		return execRedir(myargv, left, right);
	}

	// pipe exists
	int fd[2];
	if (pipe(fd) < 0) {
		return PIPE_ERR;
	}
	int result = NORMAL_EXIT;
	pid_t pid = fork();
	if (pid < 0) {
		result = FORK_ERR;
	} else if (pid == 0) { // child process
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO); // stdout redirected to fd[1]
		close(fd[1]);
		// consider redirection
		result = execRedir(myargv, left, pipeIndex);
		exit(result);
	} else { // parent process
		int status;
		waitpid(pid, &status, 0);
		if (pipeIndex + 1 < right) {
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO); // stdin redirected to fd[0]
			close(fd[0]);
			// recursively execute following command(s)
			result = execPipe(myargv, pipeIndex + 1, right);
		} else if (!WIFEXITED(status)) {
			result = CHILD_PROCESS_ERR;
		}
	}

	return result;
}

// index range for command tokens: [left, right)
// redirection possibly included
// pipe already excluded
int execRedir(char** myargv, int left, int right) {
	// check if redirection exists
	int in_redir_flag = 0;
	int out_redir_flag = 0;
	int out_redir_append_flag = 0;
	int background_flag = 0;
	char *infile = NULL, *outfile = NULL;
	int endIndex = right;

	for (int i = left; i < right; ++i) {
		if (strcmp(myargv[i], "<") == 0) { // in-redirection
			in_redir_flag = 1;
			if (i + 1 < right) {
				infile = myargv[i + 1];
				endIndex = i;
				break;
			} else {
				return PARAM_MISSING_ERR;
			}
		} else if (strcmp(myargv[i], ">") == 0) { // out-redirection
			out_redir_flag = 1;
			if (i + 1 < right) {
				outfile = myargv[i + 1];
				endIndex = i;
				break;
			} else {
				return PARAM_MISSING_ERR;
			}
		} else if (strcmp(myargv[i], ">>") == 0) { // out-redirection-appending
			out_redir_append_flag = 1;
			if (i + 1 < right) {
				outfile = myargv[i + 1];
				endIndex = i;
				break;
			} else {
				return PARAM_MISSING_ERR;
			}
		} else if (strcmp(myargv[i], "&") == 0) { // background
			background_flag = 1;
			endIndex = i;
			break;
		}
	}

	// redirection details
	int result = NORMAL_EXIT;
	pid_t pid = fork();
	if (pid < 0) {
		result = FORK_ERR;
	} else if (pid == 0) {
		// open input/output file for in-/out-redirection
		int fd;
		if (in_redir_flag == 1) {
			fd = open(infile, O_RDONLY, 0777);
			dup2(fd, STDIN_FILENO);
			close(fd);
		}
		if (out_redir_flag == 1) {
			fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		if (out_redir_append_flag == 1) {
			fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0777);
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}

		// copy the command tokens to char** cmd
		// keep the oringinal command unchanged
		char* cmd[32];
		for (int i = left; i < endIndex; ++i) {
			cmd[i] = myargv[i];
		}
		cmd[endIndex] = NULL;

		execvp(cmd[left], cmd + left);
		exit(0);
	} else {
		// waitpid() is called only for non-background running
		if (!background_flag) {
			int status;
			waitpid(pid, &status, 0);
			if (!WIFEXITED(status)) {
				result = CHILD_PROCESS_ERR;
			}
		}
	}

	return result;
}
