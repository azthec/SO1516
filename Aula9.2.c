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
#include <math.h>

//COMPILA COM gcc -pthread SO8.3.1.c
//atoi recebe string retorna int

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

long long *create_vector1(int);
long long *create_vector2(int);
void do_work(void);
void produto_interno(int, int);

int threads, vector_size, val;
long long prod_int, *vector_a, *vector_b;


int round_div(int dividend, int divisor) {
    return (dividend + (divisor / 2)) / divisor;
}

void *fazThread(void *void_counter_ptr) {
    int *i_ptr = (int *)void_counter_ptr;
    int minval;
    int maxval;
    if(*i_ptr==threads-1) {
      int rest = vector_size % threads;
      minval = (*i_ptr)*val;
      maxval = minval + val + rest;
    } else {
      minval = (*i_ptr)*val;
      maxval = minval+val;
    }
    produto_interno(minval,maxval);
}

int main(int argc, char *argv[]) {
  vector_size = atoi(argv[1]);
  threads = atoi(argv[2]);
  vector_a = create_vector1(vector_size);
  vector_b = create_vector2(vector_size);
  do_work();

  val = round_div(vector_size,threads);

  
  int i = 0;
  pthread_t * thread = malloc(sizeof(pthread_t)*threads);
  for (i = 0; i < threads; i++) {
    if(pthread_create(&thread[i], NULL, fazThread, &i)){
      perror("error");
      exit(EXIT_FAILURE);
    }
  }
  for (i = 0; i < threads; i++) {
    if(pthread_join(thread[i], NULL)){
      perror("error");
      exit(EXIT_FAILURE);
    }
  }



  return EXIT_SUCCESS;
}

long long *create_vector1(int size) {
  int i;
  long long *vector;

  vector = (long long *) malloc(size * sizeof(long long));
  for (i = 0; i < size; i++)
    vector[i] = i;	
  return vector;
}

long long *create_vector2(int size) {
  int i;
  long long *vector;

 vector = (long long *) malloc(size * sizeof(long long));
  for (i = 0; i < size; i++)
    vector[i] = size-i-1;	
  return vector;
}

void do_work(void) {
  prod_int = 0;
  produto_interno(0, vector_size);
  printf("Produto Interno = %lld\n", prod_int);
  return;
}

void produto_interno(int start, int end) {
  int i;

  for (i = start; i < end; i++) {
    pthread_mutex_lock(&mutex);
    prod_int += vector_a[i] * vector_b[i];
    pthread_mutex_unlock(&mutex);
  }
  return;
}