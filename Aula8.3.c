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

#define SEM_NAME "/semaforo"

//COMPILA COM gcc -pthread SO8.3.1.c


int main(int argc, char* argv[]) {

	pid_t pid;

	sem_t *semaforo;

	semaforo = sem_open(SEM_NAME,O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);

	if((pid=fork())==-1) {
        perror("error");
        exit(EXIT_FAILURE);
    }

    if(pid) {
    	printf("pai\n");
    	sem_post(semaforo);
    } else {
    	sem_wait(semaforo);
    	printf("filho\n");
    }
}