#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void main(int argc, char * argv[]){
        DIR *dir;
        struct dirent *dp;
        if(argv[1]==NULL){
                if ((dir = opendir (".")) == NULL) {
                        perror ("Cannot open .");
                        exit (1);
                }
        }else{
                if ((dir = opendir (argv[1])) == NULL) {
                        perror ("Cannot open dir");
                        exit (1);
                }
        }
        while((dp=readdir(dir))!= NULL){
                        printf("Nome: %s\n",dp->d_name);
                }
        closedir(dir);

}
