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
#include <semaphore.h>


    int recebi_sinal = 0;

void child_handler(int sig)
{
  if (sig == SIGUSR1)
    recebi_sinal=1;
}

int main(int argc, char* argv[]) {

	pid_t pid;


    signal(SIGUSR1,child_handler);

    if((pid=fork())==-1) {
        perror("error");
        exit(EXIT_FAILURE);
    }
    
    if(pid) { //pai
        printf("pai\n");
        if(kill(pid, SIGUSR1))
            perror("error\n");
    } else { //filho
        while(!recebi_sinal) { //garante que o filho espera pelo pai

        }
        printf("filho!\n");
    }
}

/*
    esta resolucao nao funciona sempre porque caso o pai execute antes do filho chegar ao pause
    o pai envia o sinal e o filho ainda nao esta pronto para o receber


	if((pid=fork())==-1) {
        perror("error");
        exit(EXIT_FAILURE);
    }
	
    if(pid) { //pai
        printf("pai\n");
        if(kill(pid, SIGUSR1))
            perror("error\n");
    } else { //filho
        signal(SIGUSR1,child_handler);
        pause();
    }
*/

