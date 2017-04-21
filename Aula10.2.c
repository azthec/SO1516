
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


void main(int argc, char * argv[]){
        DIR *dir;
        struct dirent *dp;
        struct stat sb;
        int         status;

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
                        char yolo[512];
                        printf("Nome: %s\n",dp->d_name);
                        if(argv[1]!=NULL){
                                strcpy(yolo,argv[1]);
                                strcat(yolo,"/");
                                strcat(yolo,dp->d_name);
                        }else{
                                strcpy(yolo,"/");
                        }
                        status= lstat(yolo,&sb);
                        printf("I-node number:            %ld\n", (long) sb.st_ino);
                        printf("Mode:                     %lo (octal)\n",(unsigned long) sb.st_mode);
                        printf("Link count:               %ld\n", (long) sb.st_nlink);
                        printf("Ownership:                UID=%ld   GID=%ld\n",(long) sb.st_uid, (long) sb.st_gid);
                        printf("Preferred I/O block size: %ld bytes\n",(long) sb.st_blksize);
                        printf("File size:                %lld bytes\n",(long long) sb.st_size);
                        printf("Blocks allocated:         %lld\n",(long long) sb.st_blocks);
                        printf("\n");

                }
        closedir(dir);

}
