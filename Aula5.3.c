#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
/*
ARGUMENTOS
./a.out INPUTFILE.TXT OUTPUTFILE.TXT KEYWORD


PAI
Faz ficheiro input default input descriptor
Faz pipewrite default output descriptor

*/

int main(int argc, char *argv[])
{
  char string[80];
  char readbuffer[80];
  int pid, link[2];
  int fd[2];
  if(argc != 4)
  {
    printf("Syntax: %s [input_file_name] [output_file_name] [keyword]\n", argv[0]);
    return 0;
  }
  /*
  if((fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) <0 ) 
  {
    printf("Error:file error %s %s\n", argv[1], argv[2]);
    return 0;
  }
  //cheguei aqui, ficheiro aberto com sucesso
  */
  if(pipe(fd) == -1)
  {
    printf("Error:pipe error\n");
    exit(0);
  }
  // cria child, pos guardado em pid
  if((pid=fork()) == -1)
  {
   printf("Error: fork error\n");
    exit(0);
  }


  //fd[0] read, fd[1] write
  
  //parent
  if (pid)
  {
	int filein;
	if((filein = open(argv[1], O_RDONLY))< 0) {
    //erro
  }

	dup2(filein, STDIN_FILENO); // Replace stdin with the read end of the pipe
	dup2(fd[1],STDOUT_FILENO);
	close(fd[1]); // Don't need another copy of the pipe read end hanging about
	close(fd[0]);
	close(filein);
	if(execlp("cat","cat",NULL) < 0) {
		//exec error
	}
	/*
	close(fd[1]);
	wait(NULL); //porque é que so funciona com o wait null???
	*/
  }

  //child
  else
  {
	int fileout;
	if((fileout = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG )) < 0) {
    //erro
  }
	dup2(fd[0],STDIN_FILENO); //to the stdin
	dup2(fileout,STDOUT_FILENO);
	close(fd[1]); // Don't need another copy of the pipe read end hanging about
	close(fd[0]);
  close(fileout);
	if(execlp("grep","grep",argv[3],NULL) < 0) {
		//exec error
	}
  }
  return 0;
}
