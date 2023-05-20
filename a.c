#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SECONDS_TO_TERMINATE 5

void handle_sigusr1(int sig) {
    printf("Program A has received a signal from program B!\n");
}

int main() {
    pid_t child_pid, w;
    int status;//it will help us with analyzing the default case
    //we'll be catching the signal from program B with the help of handle_sigusr1()
    signal(SIGUSR1, handle_sigusr1);

    switch (child_pid = fork()) {
    case -1:
        perror("fork");
        exit(EXIT_FAILURE);
    case 0:
        //this is where the launch of the program B takes place
        char* progs[] = { "./b", NULL};
        execvp(progs[0], progs);
        break;
    default:
        pause();//pause() causes the calling process to sleep until a signal is delivered
        sleep(SECONDS_TO_TERMINATE);
        kill(child_pid, SIGTERM);
        wait(&status);////for getting information about a descendant whose condition has changed
        break;
    }
    exit(EXIT_SUCCESS);
}
