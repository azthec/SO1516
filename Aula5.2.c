#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
	char string[80];
	char readBuffer[80];
	int fd[2], pid;
	
	 if(pipe(fd) == -1)
	{
		printf("Erro no pipe\n");
		exit(0);
	}
	// cria child, pos guardado em pid
	if((pid=fork()) == -1)
	{
		printf("Erro no fork\n");
		exit(0);
	}
	
	//fd[0] read, fd[1] write
	//parent
	if(pid) {
		close(fd[0]);
		int i;
		for(i = 1; i<=100;i++) {
			//strcpy(string, "Linha ");
			sprintf(string, "Linha %d\n", i);
			write(fd[1], string, strlen(string));
		}
		close(fd[1]);
		wait(NULL);
	} 
	//child
	else {
		close(fd[1]);
		dup2(fd[0],STDIN_FILENO); //to the stdin
		close(fd[0]);
		if(execlp("more","more",NULL) < 0) {
			//exec error
		}
	}
	
}