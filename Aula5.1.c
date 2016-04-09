#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

/*
if((fd=open("file_hole.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
		//error {
	} 
		write(fd,buf,10); //offset now 10
		lseek(fd,80,SEEK_SET); //offset now 80
		write(fd,buf,10); //offset now 90
		
	}
*/


int main(int argc, char *argv[])
{
	char string[80];
	char readBuffer[80];
	//FILE *fp;
	int fd[2], pid;
	/*
	if(argc!=2) {
		printf("Sintaxe: %s [ficheiro]\n", argv[0]);
	}
	fp = fopen(argv[1], "r");
	if(!fp)
	{
		printf("Erro no ficheiro\n", argv[1]);
		return 0;
	}
	*/
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
		write(fd[1], "hello world\n", 12);
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