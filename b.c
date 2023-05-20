#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_STR_ELEM 32768
#define PROG 32
#define PROG_ARGS 1024

pid_t pid;

void handle_sigterm(int sig) {
    printf("Program B was terminated...\n");
    kill(-pid, SIGTERM);//"-", because if the -pid value is less than -1, 
    //then a signal is sent to each process that is part of a group of processes whose ID is equal to pid
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    char buffer[MAX_STR_ELEM];
    //let's create a space to store incoming commands (programs) and its args (parameters)
    char* matrix_without_pipe[PROG][PROG_ARGS];
    
    pid = getpid();

    //if the process ends unexpectedly, the program sends a signal
    signal(SIGTERM, handle_sigterm);

    printf("Enter the programs and their parameters:\n");
    fgets(buffer, MAX_STR_ELEM, stdin);//here we've just stored the string into buffer

    //this whole part is responsible for spliting commands, args and "|" and storing them into matrix (without "|")
    int number_of_string = 0, number_of_column = 0;
    char* input_string = strtok(buffer, " \n");//a function that splits words by spaces and \n
    matrix_without_pipe[number_of_string][number_of_column] = input_string;
    ++number_of_column;
    while (input_string != NULL) {
        input_string = strtok(NULL, " \n");
        if (input_string != NULL) {
            if (strcmp(input_string, "|") != 0) {//a function that compares input_string with "|"
                matrix_without_pipe[number_of_string][number_of_column] = input_string;
                ++number_of_column;
            }
            else {
                ++number_of_string;
                number_of_column = 0;
            }
        }
    }

    //we use kill() to send a signal to program A
    kill(getppid(), SIGUSR1);
    printf("Program B sent a signal to program A!\n");
    //BTW: getppid() - id of destination process (in our case - pid of main process)
    //SIGUSR1 - the type of signal to send (in our case - user-defined signal)

    int fd[number_of_string][2];//an array of file descriptors (fd)
    //fd[number_of_string][0] - indicates the end of the read channel
    //fd[number_of_string][1] - indicates the end of the write channel
    for (int i = 0; i < number_of_string; i++) {
        if (pipe(fd[i]) == -1) {//creating a pipe - a channel that can be used for inter-process communication
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    //linking streams of pipes: the output of n must be passed to the input of n+1
    for (int i = 0; i <= number_of_string; i++) {
        pid_t pid = fork();//the fork makes the both processes have access to both ends of the pipe
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            if (number_of_string != 0) {
                if (i == 0) {
                    if (dup2(fd[i][1], 1) == -1) {//redirecting standart output to the fd[i][1]
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (i == number_of_string) {
                    if (dup2(fd[i - 1][0], 0) == -1) {//redirecting standart input to the fd[i-1][0]
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    if (dup2(fd[i - 1][0], 0) == -1) {//redirecting standart input to the fd[i-1][0]
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(fd[i][1], 1) == -1) {//redirecting standart output to the fd[i][1]
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }

                //if we duplicate one end of a pipe to standard input or standard output,
                //we should close both ends of the original pipe before using execvp()
                for (int j = 0; j < number_of_string; j++) {
                    close(fd[j][0]);
                    close(fd[j][1]);
                }
            }

            //well, here in matrix_without_pipe[i][0] we have all the programs (commands) like <prog1>, <prog2> etc.
            //matrix_without_pipe[i] - is basically an array like <progi> <progi args>
            
            //the exec() family of functions replaces the current process image with a new process image
            //so in the next line we want to execute matrix_without_pipe[i][0] with arguments matrix_without_pipe[i]
            if (execvp(matrix_without_pipe[i][0], matrix_without_pipe[i]) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    }
    //closing both ends of the pipes once again
    for (int i = 0; i < number_of_string; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }
    //waiting for children's processes to finish their work
    for (int i = 0; i <= number_of_string; i++)
        wait(NULL);

    exit(EXIT_SUCCESS);
}
