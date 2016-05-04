#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>

void child_handler(int sig)
{
	if (sig == SIGTSTP)
    	printf("sinal SIGINT\n");
    if (sig == SIGINT)
    	printf("sinal SIGINT\n");
}


int main(int argc, char* argv[]) {

	pid_t pid;

	pid = getpid();
	printf("%d\n", pid);
	
    signal(SIGINT,child_handler);
    signal(SIGTSTP,child_handler);

    while(1) pause();


}




