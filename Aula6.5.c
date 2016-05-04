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
void child_handler(int sig);

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
      com = parse(linha);
      if (com) {
	print_parse(com);
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

void child_handler(int sig) {
	if(sig==SIGCHLD)
		waitpid(-1,NULL,WNOHANG);
}

void free_commlist(COMMAND *commlist){
    COMMAND* tmp;
    while(commlist!=NULL) {
    	tmp= commlist->next;
    	free(commlist);
    	commlist=tmp;
    }
}

void execute_commands(COMMAND *commlist) {
    int pid, fd[2], fdold[2], filepointer, status;
    COMMAND* liststart = commlist;

    if(strcmp(commlist->argv[0],"exit")==0) {
    	exit(EXIT_SUCCESS);
    }

    if(background_exec)
    	signal(SIGCHLD,child_handler);
    else
    	signal(SIGCHLD,SIG_IGN);

    while(commlist!=NULL) {
    	fdold[0]=fd[0];
    	fdold[1]=fd[1];

		if(commlist->next!=NULL) {
    		if(pipe(fd)==-1) {
    			//error
    		}
   		}

    	if((pid=fork())==-1) {
    		//error
    	}

    	if(pid) {
    		if(liststart!=commlist) {
    			close(fdold[0]);
    			close(fdold[1]);
    		}
    		commlist=commlist->next;
    	}else{
    		if(liststart!=commlist) {
    			dup2(fdold[0],0);
    			close(fdold[0]);
    			close(fdold[1]);
    		}

    		if(commlist->next!=NULL) {
    			dup2(fd[1],1);
    			close(fd[0]);
    			close(fd[1]);
    		}

    		if(commlist==liststart && inputfile != NULL) {
    			filepointer = open(inputfile,O_RDONLY);
    			if(filepointer==-1) {
    				//error
    			}
    			dup2(filepointer,0);
    			close(filepointer);
    		}

    		if(commlist->next==NULL && outputfile!=NULL) {
    			filepointer = open(outputfile,O_WRONLY | O_CREAT | O_TRUNC, 0644);
    			if(filepointer==-1) {
    				//error
    			}
    			dup2(filepointer,1);
    			close(filepointer);
    		}

    		execvp(commlist->cmd,commlist->argv);
    	}
    }

    if(background_exec==0) {
    	waitpid(pid,&status, WUNTRACED| WCONTINUED);
    }
}

