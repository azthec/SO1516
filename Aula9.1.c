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
#include <pthread.h>

#define SEM_NAME "/semaforo"
#define EXECUCOES 10

//COMPILA COM gcc -pthread SO8.3.1.c
//A PASSAR O POINTER TER CUIDADO SE DECLARO A VARIAVEL NUMA FUNCAO QUE RETORNA ANTES DO THREAD TERMINAR, A VARIAVEL DECLARADA DEIXA DE ESTAR EM SCOPE, NESSE CASO USAR GLOBAIS

sem_t *semaforo;
sem_t *semaforo2;


void *contaImpar(void *void_counter_ptr) {
    int *counter_ptr = (int *)void_counter_ptr;
    int nIteracoes = EXECUCOES;
        while(nIteracoes>0) {
           sem_wait(semaforo);
           printf("filho: %d\n",*counter_ptr);
           (*counter_ptr)++;
           nIteracoes--;
           sem_post(semaforo2);
        }
}


int main(int argc, char* argv[]) {

    int contador = 0;

    sem_unlink(SEM_NAME);
    sem_unlink("/semaforo2");

	semaforo = sem_open(SEM_NAME,O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    semaforo2 = sem_open("/semaforo2",O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);

    pthread_t *tidp;
    if(pthread_create(tidp,NULL,contaImpar,&contador)) {
        perror("error");
        exit(EXIT_FAILURE);
    }
    int nIteracoes = EXECUCOES;
    while(nIteracoes>0) {
        sem_wait(semaforo2);
        printf("pai: %d\n", contador);
    	sem_post(semaforo);
        contador++;
        nIteracoes--;
        }
}