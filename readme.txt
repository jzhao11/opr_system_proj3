I.Build Instructions:
make
By using the original Makefile (with gcc command included) in the repo, the instruction "make" (under the repo csc415-shell-program-jzhao11) can build the program and generate the executable file "myshell".



II.Run Instructions:
./myshell
This instruction is also executed under csc415-shell-program-jzhao11, which runs the executable file "myshell".



III.Assumptions in My Shell:
The current version of my shell is designed to simulate a real one. Some assumptions are made during the implementation, and therefore causing some limitations within this shell.

It is assumed that "&" appears at most once in each command. If there exists an "&", it should be only at the end of the command. And the maximum length (number of characters) for each command is (BUFFERSIZE - 1), which is 255. It can only accept and execute valid commands to achieve correct output information, where the maximum number of command tokens is 32 (ended with NULL). Besides, it is not able to response to some functional keys on the keyboard, such as "tab" (to prompt the possible file or folder names), "up" (to prompt the previous command), "down" (to prompt the next command), "left" or "right" (to move the cursor). Moreover, there has to be at least one space between ">>", ">", "<" and the file path.



IV. Implementation Discussion (What the Code Does):
The shell program can be regarded as an infinite loop, with each loop summarized as: i) prompt working directory; ii) read the input command; iii) parse the command; iv) execute the command.

The command will be read from user's input by fgets() and broken into pieces by a self-defined function parse(). Basically, parse() utilizes strtok() to split string into smaller tokens and store them in char* myargv[] as parameters, during which white spaces are used as delimiters. After parsing the input, the special command "cd" will use chdir() to change the working directory, and "exit" will trigger exit() to terminate this shell. For other normal commands, a self-defined function execCmd() is called for execution.

When executing, the command will firstly be considered as a pipe by execPipe(), since every single command (without "|") could be taken as the most basic element in a pipe. Then fork() will create a child process to call execRedir(), while parent process is responsible for the possible following command in pipe. In other words, if there still exists token(s) after "|", the recursive call of execPipe() will deal with the following command. Otherwise, pipe "|" is excluded and thus execRedir() can focus on redirection. In execRedir(), flag variables are used to determine if there exists input-redirection, output-redirection, output-redirection-appending, or background running. Specifically, the redirection is implemented by using open(), dup2(), and close(), with standard input/output redirected to the specified file. If "&" is detected, fork() will create new child process to run the command in background and the parent process will not call wait() to wait for the completion of that child process.

For extra credit question 1, the execution for combination of single commands is also implemented. The recursion method is applied to handling the shell pipe, since the number of single commands in a pipe cannot be predicted beforehand. In execPipe(), the parent process will recursively execute the following commands, if possible.

For extra credit question 2, the prompt information is modified to show the current working directory. This is done by self-defined getCurrDir() function, with getcwd() to retrieve the absolute path of current working directory and strstr() to search "/home/username" from the path and then replace it by "~".



V. Sample Tests
ls -l
cat readme.txt
ls -al /usr/src
ls -l &
ls -al /usr/src &
ls -l > outfile
ls -l >> outfile
ls -al /usr/src > outfile2
ls -al /usr/src >> outfile2
grep shell < outfile
grep linux < outfile2
ls -al /usr/src | grep linux
cd ./../
cd csc415-shell-program-jzhao11
cd /
cd /usr
pwd
ls -la | grep .c | wc -l
ls -l /usr/src | grep linux | wc -l > outfile
ls -al | grep .txt >> outfile
grep execvp < myshell.c
cat myshell.c | wc -l
exit
