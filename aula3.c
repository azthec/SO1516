//para compilar gcc <nomeficheiro> -lreadline

  #include <stdio.h>
  #include <stdlib.h>
  #include <readline/readline.h>
  #include <readline/history.h>
  #include <string.h> //strtok?

  #define MAXARGS 100
  typedef struct command {
    char *cmd;              // string apenas com o comando                                        //aponta para o comando em si, igual a argv[0]
    int argc;               // número de argumentos                                               //numero de argumentos, incluindo o comando incial
    char *argv[MAXARGS+1];  // vector de argumentos do comando                                    //comandos em si
  } COMMAND;

  COMMAND parse(char *linha);
  void print_parse(COMMAND com);

  int main() {
    char *linha;
    COMMAND com;

    while (1) {
      if ((linha = readline("my_prompt$ ")) == NULL)
			exit(0);
		if (strlen(linha) != 0) {
			add_history(linha);  //biblioteca readline, historico de comandos que escrevi
			com = parse(linha);
			if(strcmp(com.cmd ,"exit")==0)
				exit(0);		
			print_parse(com);
      }
      free(linha);
    }
  }

  COMMAND parse(char *linha) {
    COMMAND com;
    int ctr = 0;
    char* p = strtok(linha, " ");
    if(p==NULL)
		exit(0);
    com.cmd = p;
    com.argv[ctr]  = p;
    while (p!=NULL) {
		ctr++;
		p = strtok(NULL, " ");
		com.argv[ctr]  = p;
		
    } 
    com.argc=ctr;
    return com;
  }


  void print_parse(COMMAND com) {
    printf("#args: %i\nCommand:\n", com.argc);
    int i=0;
    while(com.argv[i]!=NULL){
		printf("%p: %s %c \n", com.argv[i], com.argv[i], *com.argv[i]);
		i++;
	} 
  }
