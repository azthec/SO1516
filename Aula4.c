#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void forkChildren(int numChild);
void forkSerie(int numChild);
void forkExecLS();
void forkExit();

int main(int argc, char *argv[]) {
  //printf("%d\n", '.');
  //printf("%d\n", *argv[1]-'0');
  printf("'%s' '%s'\n", argv[0], argv[1]);
  //int filhos = atoi(argv[1]);
  //forkChildren(filhos);
  //forkSerie(filhos);
  //forkExecLS();
  forkExit();
}

void forkChildren(int numChild) {
  int i;
  pid_t pid;
  for(i = 1; i<= numChild; i++) {
    pid = fork();
    if(pid==0) {
      printf("processo filho com numerp: %d | PID: %d\n",i,getpid());
      break;
    }
  }
}

void forkSerie(int numChild) {
  int i;
  pid_t pid;
  pid = fork();
  for(i =1; i<= numChild; i++ ) {
    if(pid != 0) {
      printf("parent pid %d and PPID %d.\n", getpid(), getppid());
      printf("n=%d\n",i);
      break;
    } else {
      printf("child PID %d and PPID %d.\n", getpid(), getppid());
      pid = fork();
    }
  }
}

void forkExecLS() {
  pid_t pid;
  char *argumentos[] = {"/bin/ls", "-l", 0 };
  if(fork()==0) {
    printf("iam child -- starting ls: \n");
    execv(argumentos[0], argumentos);
  }
}


void forkExit() {
  int dif;
  pid_t pid;
  pid = fork();
  if(pid!=0) {
    wait(&dif);
    printf("PARENT PID = %d\n", getpid());
    printf("pid dif = %d\n", WEXITSTATUS(dif));
  } else {
  int k = getpid() - getppid();
  printf("CHILD PID = %d\n", getpid());
  exit(k);
  }
}
