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

#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1

/*
COMPILAR__
  gcc Aula6.1.c -lreadline

ALTERAR ERROR CODES__
  perror();
  Output-> prints to screen ": SYSTEM_ERROR_SPECIFIC_STRING"
*/


typedef struct command {
    char *cmd;              // string apenas com o comando
    int argc;               // número de argumentos
    char *argv[MAXARGS+1];  // vector de argumentos do comando
    struct command *next;   // apontador para o próximo comando
} COMMAND;

// variáveis globais
char* inputfile = NULL;	    // nome de ficheiro (em caso de redireccionamento da entrada padrão)
char* outputfile = NULL;    // nome de ficheiro (em caso de redireccionamento da saída padrão)
int background_exec = 0;    // indicação de execução concorrente com a mini-shell (0/1)

// declaração de funções
COMMAND* parse(char* linha);
void print_parse(COMMAND* commlist);
void execute_commands(COMMAND* commlist);
void free_commlist(COMMAND* commlist);
int firstcommand = 1;
//int lastcomand; desnecessario, substituido por (commlist-> == NULL)

// include do código do parser da linha de comandos
#include "parser.c"

int main(int argc, char* argv[]) {
  char *linha;
  COMMAND *com;

  while (1) {
    if ((linha = readline("my_prompt$ ")) == NULL)
      exit(0);
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha); //objecto command == input da linha
      if (com) {
	       //print_parse(com);
	       execute_commands(com);
	       free_commlist(com);
      }
    }
    free(linha);
  }
}


void print_parse(COMMAND* commlist) {
  int n, i;

  printf("---------------------------------------------------------\n");
  printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
  n = 1;
  while (commlist != NULL) {
    printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
    i = 0;
    while (commlist->argv[i] != NULL) {
      printf("%s,", commlist->argv[i]);
      i++;
    }
    printf("%s'\n", commlist->argv[i]);
    commlist = commlist->next;
    n++;
  }
  printf("---------------------------------------------------------\n");
}

void free_commlist(COMMAND *commlist){
    // ...
    // Esta função deverá libertar a memória alocada para a lista ligada.
    // ...
  COMMAND* com;
  while(commlist != NULL) {
    com = commlist->next;
    free(commlist);
    commlist = com;
  }
  background_exec=0;
}

void execute_commands(COMMAND *commlist) {
    // ...
    // Esta função deverá "executar" a "pipeline" de comandos da lista commlist.
    // ...
  if(strcmp(commlist->argv[0],"exit")==0) {
    exit(EXIT_SUCCESS);
  }

  pid_t pid;

  int fd[2];
  int oldfd[2];

  while(commlist!=NULL) {
    oldfd[0]=fd[0];
    oldfd[1]=fd[1];
    // crio pipe, para partilhar pai e filho
    if(pipe(fd) == -1) {
      perror("Error");
      exit(0);
    }

    // cria child, pos guardado em pid
    
    if((pid=fork()) == -1) {
      perror("Error");
      exit(0);
    }

    //fd[0] read, fd[1] write

    //parent
    if(pid) {
      //o pai so serve para contar firstcommand e fechar pipes antigos
      if(firstcommand==1) firstcommand=0;
      else close(oldfd[0]); //já foi executado fecha read antigo

      //ja foi executado fecha fd escrita
      close(fd[1]);

      if(commlist->next==NULL) {
        close(fd[0]);
      }

      commlist = commlist->next; //IR PARA O PROXIMO COMANDO

      wait(NULL);
    } 
    //child
    else {
      if(inputfile != NULL && firstcommand == 1) {
        int file;
          /* Abre o ficheiro e guarda file descriptor */
          if ((file = open(inputfile, O_RDONLY)) < 0 ) {
            perror("Error");
            exit(EXIT_FAILURE);
          }
          /* Duplica o file descriptor e substitui pelo STDIN */
          dup2(file, STDIN_FILENO); //dup2(source,destino)
      }

      //se nao e o primeiro comando le do processo pai
      if(firstcommand==0) {
        dup2(oldfd[0],0);
      }

      if(outputfile!=NULL && commlist->next != NULL) {
        int file;
        /* Abre o ficheiro e guarda o file descriptor */
        if ((file = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
            perror("Error");
            exit(EXIT_FAILURE);
          }
          /* Duplica o file descriptor e substitui pelo STDOUT */
          dup2(file, STDOUT_FILENO); //dup2(source,destino)

      }

      //se ha mais comandos isto escreve na pipe
      if(commlist->next!=NULL) {
        dup2(fd[1],STDOUT_FILENO);
      }

      if(execvp(commlist->cmd,commlist->argv) < 0) { //versao generica do execlp, 2 argumentos execvp(comando,vector com argumentos)
        //exec error
       perror("Error");
       exit(EXIT_FAILURE);
      }

    }
  }

} 


