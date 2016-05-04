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
    sem_t *semaforo2;

    int nIteracoes = 10; //conta ate 20

    sem_unlink(SEM_NAME);
    sem_unlink("/semaforo2");


	semaforo = sem_open(SEM_NAME,O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    semaforo2 = sem_open("/semaforo2",O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);

	if((pid=fork())==-1) {
        perror("error");
        exit(EXIT_FAILURE);
    }

    if(pid) {
        int contador = 1;
        while(nIteracoes>0) {
            sem_wait(semaforo2);
            printf("pai: %d\n", contador);
        	sem_post(semaforo);
            contador+=2;
            nIteracoes--;
        }
    } else {
        int contador = 2;
        while(nIteracoes>0) {
    	   sem_wait(semaforo);
    	   printf("filho: %d\n",contador);
           contador+=2;
           nIteracoes--;
           sem_post(semaforo2);
        }
    }
}