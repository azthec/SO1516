#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXARGS 100
#define CHECK_NUMBER 9999
#define TYPE_DIR 'D'
#define TYPE_FILE 'F'
#define MAX_NAME_LENGHT 20

#define FAT_ENTRIES(TYPE) (TYPE == 8 ? 256 : TYPE == 10 ? 1024 : 4096)
#define FAT_SIZE(TYPE) (FAT_ENTRIES(TYPE) * sizeof(int))
#define BLOCK(N) (blocks + N * sb->block_size)
#define DIR_ENTRIES_PER_BLOCK (sb->block_size / sizeof(dir_entry))

typedef struct command {
  char *cmd;              // string apenas com o comando
  int argc;               // número de argumentos
  char *argv[MAXARGS+1];  // vector de argumentos do comando
} COMMAND;

typedef struct superblock_entry {
  int check_number;   // número que permite identificar o sistema como válido
  int block_size;     // tamanho de um bloco {256, 512(default) ou 1024 bytes}
  int fat_type;       // tipo de FAT {8, 10(default) ou 12}
  int root_block;     // número do 1º bloco a que corresponde o directório raiz
  int free_block;     // número do 1º bloco da lista de blocos não utilizados
  int n_free_blocks;  // total de blocos não utilizados
} superblock;

typedef struct directory_entry {
  char type;                   // tipo da entrada (TYPE_DIR ou TYPE_FILE)
  char name[MAX_NAME_LENGHT];  // nome da entrada
  unsigned char day;           // dia em que foi criada (entre 1 e 31)
  unsigned char month;         // mes em que foi criada (entre 1 e 12)
  unsigned char year;          // ano em que foi criada (entre 0 e 255 - 0 representa o ano de 1900)
  int size;                    // tamanho em bytes (0 se TYPE_DIR)
  int first_block;             // primeiro bloco de dados
} dir_entry;

// variáveis globais
superblock *sb;   // superblock do sistema de ficheiros
int *fat;         // apontador para a FAT
char *blocks;     // apontador para a região dos dados
int current_dir;  // bloco do directório corrente

// funções auxiliares
COMMAND parse(char*);
void parse_argv(int, char*[]);
void init_filesystem(int, int, char*);
void init_superblock(int, int);
void init_fat(void);
void init_dir_block(int, int);
void init_dir_entry(dir_entry*, char, char*, int, int);
void exec_com(COMMAND);

// funções de manipulação de directórios
void vfs_ls(void);
void vfs_mkdir(char*);
void vfs_cd(char*);
void vfs_pwd(void);
void vfs_rmdir(char*);

// funções de manipulação de ficheiros
void vfs_get(char*, char*);
void vfs_put(char*, char*);
void vfs_cat(char*);
void vfs_cp(char*, char*);
void vfs_mv(char*, char*);
void vfs_rm(char*);


int main(int argc, char *argv[]) {
  char *linha;
  COMMAND com;

  parse_argv(argc, argv);
  while (1) {
    if ((linha = readline("vfs$ ")) == NULL)
      exit(0);
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha);
      exec_com(com);
    }
    free(linha);
  }
  return 0;
}


COMMAND parse(char *linha) {
  int i = 0;
  COMMAND com;

  com.cmd = strtok(linha, " ");
  com.argv[0] = com.cmd;
  while ((com.argv[++i] = strtok(NULL, " ")) != NULL);
  com.argc = i;
  return com;
}


void parse_argv(int argc, char *argv[]) {
  int i, block_size, fat_type;

  block_size = 512; // valor por omissão
  fat_type = 10;    // valor por omissão
  if (argc < 2 || argc > 4) {
    printf("vfs: invalid number of arguments\n");
    printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
    exit(1);
  }
  for (i = 1; i < argc - 1; i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'b') {
        block_size = atoi(&argv[i][2]);
        if (block_size != 256 && block_size != 512 && block_size != 1024) {
          printf("vfs: invalid block size (%d)\n", block_size);
          printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
          exit(1);
        }
      } else if (argv[i][1] == 'f') {
        fat_type = atoi(&argv[i][2]);
        if (fat_type != 8 && fat_type != 10 && fat_type != 12) {
          printf("vfs: invalid fat type (%d)\n", fat_type);
          printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
          exit(1);
        }
      } else {
        printf("vfs: invalid argument (%s)\n", argv[i]);
        printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
        exit(1);
      }
    } else {
      printf("vfs: invalid argument (%s)\n", argv[i]);
      printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
      exit(1);
    }
  }
  init_filesystem(block_size, fat_type, argv[argc-1]);
  return;
}


void init_filesystem(int block_size, int fat_type, char *filesystem_name) {
  int fsd, filesystem_size;

  if ((fsd = open(filesystem_name, O_RDWR)) == -1) {
    // o sistema de ficheiros não existe --> é necessário criá-lo e formatá-lo
    if ((fsd = open(filesystem_name, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU)) == -1) {
      printf("vfs: cannot create filesystem (%s)\n", filesystem_name);
      printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
      exit(1);
    }

    // calcula o tamanho do sistema de ficheiros
    filesystem_size = block_size + FAT_SIZE(fat_type) + FAT_ENTRIES(fat_type) * block_size;
    printf("vfs: formatting virtual file-system (%d bytes) ... please wait\n", filesystem_size);

    // estende o sistema de ficheiros para o tamanho desejado
    lseek(fsd, filesystem_size - 1, SEEK_SET);
    write(fsd, "", 1);

    // faz o mapeamento do sistema de ficheiros e inicia as variáveis globais
    if ((sb = (superblock *) mmap(NULL, filesystem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fsd, 0)) == MAP_FAILED) {
      printf("vfs: cannot map filesystem (mmap error)\n");
      close(fsd);
      exit(1);
    }
    fat = (int *) ((unsigned long int) sb + block_size);
    blocks = (char *) ((unsigned long int) fat + FAT_SIZE(fat_type));

    // inicia o superblock
    init_superblock(block_size, fat_type);

    // inicia a FAT
    init_fat();

    // inicia o bloco do directório raiz '/'
    init_dir_block(sb->root_block, sb->root_block);
  } else {
    // calcula o tamanho do sistema de ficheiros
    struct stat buf;
    stat(filesystem_name, &buf);
    filesystem_size = buf.st_size;

    // faz o mapeamento do sistema de ficheiros e inicia as variáveis globais
    if ((sb = (superblock *) mmap(NULL, filesystem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fsd, 0)) == MAP_FAILED) {
      printf("vfs: cannot map filesystem (mmap error)\n");
      close(fsd);
      exit(1);
    }
    fat = (int *) ((unsigned long int) sb + sb->block_size);
    blocks = (char *) ((unsigned long int) fat + FAT_SIZE(sb->fat_type));

    // testa se o sistema de ficheiros é válido
    if (sb->check_number != CHECK_NUMBER || filesystem_size != sb->block_size + FAT_SIZE(sb->fat_type) + FAT_ENTRIES(sb->fat_type) * sb->block_size) {
      printf("vfs: invalid filesystem (%s)\n", filesystem_name);
      printf("Usage: vfs [-b[256|512|1024]] [-f[8|10|12]] FILESYSTEM\n");
      munmap(sb, filesystem_size);
      close(fsd);
      exit(1);
    }
  }
  close(fsd);

  // inicia o directório corrente
  current_dir = sb->root_block;
  return;
}


void init_superblock(int block_size, int fat_type) {
  sb->check_number = CHECK_NUMBER;
  sb->block_size = block_size;
  sb->fat_type = fat_type;
  sb->root_block = 0;
  sb->free_block = 1;
  sb->n_free_blocks = FAT_ENTRIES(fat_type) - 1;
  return;
}


void init_fat(void) {
  int i;

  fat[0] = -1;
  for (i = 1; i < sb->n_free_blocks; i++)
    fat[i] = i + 1;
  fat[sb->n_free_blocks] = -1;
  return;
}


void init_dir_block(int block, int parent_block) {
  dir_entry *dir = (dir_entry *) BLOCK(block);
  // o número de entradas no directório (inicialmente 2) fica guardado no campo size da entrada "."
  init_dir_entry(&dir[0], TYPE_DIR, ".", 2, block);
  init_dir_entry(&dir[1], TYPE_DIR, "..", 0, parent_block);
  return;
}


void init_dir_entry(dir_entry *dir, char type, char *name, int size, int first_block) {
  time_t cur_time = time(NULL);
  struct tm *cur_tm = localtime(&cur_time);

  dir->type = type;
  strcpy(dir->name, name);
  dir->day = cur_tm->tm_mday;
  dir->month = cur_tm->tm_mon + 1;
  dir->year = cur_tm->tm_year;
  dir->size = size;
  dir->first_block = first_block;
  return;
}


void exec_com(COMMAND com) {
  // para cada comando invocar a função que o implementa
  if (!strcmp(com.cmd, "exit"))
    exit(0);
  if (!strcmp(com.cmd, "ls")) {
    // falta tratamento de erros
    vfs_ls();
  } else if (!strcmp(com.cmd, "mkdir")) {
    // falta tratamento de erros
    vfs_mkdir(com.argv[1]);
  } else if (!strcmp(com.cmd, "cd")) {
    // falta tratamento de erros
    vfs_cd(com.argv[1]);
  } else if (!strcmp(com.cmd, "pwd")) {
    // falta tratamento de erros
    vfs_pwd();
  } else if (!strcmp(com.cmd, "rmdir")) {
    // falta tratamento de erros
    vfs_rmdir(com.argv[1]);
  } else if (!strcmp(com.cmd, "get")) {
    // falta tratamento de erros
    vfs_get(com.argv[1], com.argv[2]);
  } else if (!strcmp(com.cmd, "put")) {
    // falta tratamento de erros
    vfs_put(com.argv[1], com.argv[2]);
  } else if (!strcmp(com.cmd, "cat")) {
    // falta tratamento de erros
    vfs_cat(com.argv[1]);
  } else if (!strcmp(com.cmd, "cp")) {
    // falta tratamento de erros
    vfs_cp(com.argv[1], com.argv[2]);
  } else if (!strcmp(com.cmd, "mv")) {
    // falta tratamento de erros
    vfs_mv(com.argv[1], com.argv[2]);
  } else if (!strcmp(com.cmd, "rm")) {
    // falta tratamento de erros
    vfs_rm(com.argv[1]);
  } else
    printf("ERROR(input: command not found)\n");
  return;
}


// ls - lista o conteúdo do directório actual
void vfs_ls(void) {
        dir_entry *dir = (dir_entry *) BLOCK(current_dir);
        int n_entradas=dir[0].size;
        int i;
        for(i=0;i<n_entradas;i++){
                printf("%s\n",dir[i].name);
        }

        return;
}

int find_free_block(){
        int target_block = sb->free_block;
        sb->free_block = fat[target_block];
        sb->n_free_blocks--;
        fat[target_block] = -1;

        return target_block;
}
void put_free_block(int nome){
        //int blockz=nome[0].first_block;
        fat[nome]=sb->free_block;
        sb->free_block=block;
        sb->n_free_blocks++;
        return;
}

// mkdir dir - cria um subdirectório com nome dir no directório actual
void vfs_mkdir(char *nome_dir) {
        int livre;

        dir_entry *dir = (dir_entry *)BLOCK( current_dir );
        int n_entradas=dir[0].size;
        int i;
        for(i=0; i<n_entradas; i++) {
                if(strcmp(dir[i].name, nome_dir)==0) {
                        printf("Diretorio %s ja existe",dir[i].name);
                        return;
                }
        }
        livre= find_free_block();
        init_dir_block(livre,current_dir);
        init_dir_entry(&dir[n_entradas],TYPE_DIR,nome_dir,0,livre);
        dir[0].size++;

        return;
}


// cd dir - move o directório actual para dir.
void vfs_cd(char *nome_dir) {
                dir_entry *dir= (dir_entry *) BLOCK(current_dir);
        int n_entradas= dir[0].size;
        int i;
        for(i=0; i<n_entradas; i++) {
                if(strcmp(dir[i].name, nome_dir)==0) {
                        if(dir[i].type==TYPE_DIR){
                                current_dir=dir[i].first_block;
                                return;
                }}
        }
        printf("file not found!\n");
        return;
}



void vfs_pwd_aux(dir_entry *entry,int nedex)
{
        int b;
        unsigned int i;
        dir_entry *aux;
        //dir_entry *dir = (dir_entry *)BLOCK( current_dir );
        //=entry[0].size;
        nedex++;

          /* se o directorio Ã© a raiz --> regressa */
        if( entry->first_block == sb->root_block ){
                return;
        }
        aux = (dir_entry *)BLOCK( entry->first_block );
        vfs_pwd_aux(&aux[1],nedex); // entrada ".." de `entry'

        for( b = aux[1].first_block; b != -1; b = fat[b] )
        {
          aux = (dir_entry *)BLOCK( b );
        int n_entradas=aux[0].size;
          for( i = 0; i < n_entradas; i++){

                  if( aux[i].first_block == entry->first_block )
                  {
                        printf("/%s", aux[i].name);
                        return;
                  }

         }}
}
// pwd - escreve o caminho absoluto do directório actual
void vfs_pwd(void) {
        dir_entry *dir = (dir_entry *)BLOCK( current_dir );
  if(dir->first_block == sb->root_block)
      printf("/");
  else
      vfs_pwd_aux(dir,0);
  printf("\n");
}

// rmdir dir - remove o subdirectório dir (se vazio) do directório actual
void vfs_rmdir(char *nome_dir) {
        dir_entry *dir = (dir_entry *)BLOCK( current_dir );
        int n_entradas=dir[0].size;
        int i;
        for(i=0; i<n_entradas; i++) {
                if(strcmp(dir[i].name, nome_dir)==0 && dir[i].type==TYPE_DIR) {
                        dir_entry *aux = (dir_entry *)BLOCK(dir[i].first_block);
                        if (aux[0].size != 2){
                                printf("File not empty");
                                return;
                        }else{
                                put_free_block(dir[i].first_block);
                                dir[0].size--;
                                memcpy(&dir[i],&dir[n_entradas-1],sizeof(dir_entry));
                        }
                }
        }
        return;
}


// get fich1 fich2 - copia um ficheiro normal UNIX fich1 para um ficheiro no nosso sistema fich2
void vfs_get(char *nome_orig, char *nome_dest) {
                int filein = open(nome_orig, O_RDWR);

        dir_entry *currdir = (dir_entry *) BLOCK(current_dir);

        struct stat statbuf;

        lstat(nome_orig, &statbuf);

        int tamanho = statbuf.st_size;

        int num_blocos = tamanho/sb->block_size;
        if(tamanho%sb->block_size != 0) num_blocos++;
        if(num_blocos > sb->n_free_blocks){
                printf("ERROR: no space left on device\n");
                return;
        }
        int fblock = find_free_block();
        int n_entradas = currdir[0].size;
        init_dir_entry(&currdir[n_entradas], TYPE_FILE, nome_dest, tamanho, fblock);
        currdir[0].size++;
        while(tamanho > sb->block_size){
                read(filein, BLOCK(fblock), sb->block_size);
                tamanho-= sb->block_size;
                fat[fblock] = find_free_block();
                fblock = fat[fblock];
        }
        read(filein, BLOCK(fblock), sb->block_size);
        fat[fblock] = -1;
  return;
}


// put fich1 fich2 - copia um ficheiro do nosso sistema fich1 para um ficheiro normal UNIX fich2
void vfs_put(char *nome_orig, char *nome_dest) {
        dir_entry *dir = (dir_entry*)BLOCK(current_dir);
        int size = dir[0].size;
        int fd = open(nome_dest, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if(fd == -1){ perror("Impossivel abrir o ficheiro de destino.\n"); return; }
        int i;
        for(i=0; i<size; i++){
                if(!strcmp(nome_orig, dir[i].name)){
                        if(dir[i].type == TYPE_DIR){ perror("Os dados de origem correspondem a um diretorio.\n"); return; }
                        int block = dir[i].first_block;
                        int file_size = dir[i].size;
                        while(file_size > 0){
                                if(file_size > sb->block_size) file_size -= write(fd, BLOCK(block), sb->block_size);
                                else file_size -= write(fd, BLOCK(block), file_size);
                                block = fat[block];
                        }
                        return;
                }
        }

        perror("O ficheiro de origem nao exite.\n"); return;


}


// cat fich - escreve para o ecrã o conteúdo do ficheiro fich
void vfs_cat(char *nome_fich) {


        dir_entry *entry;
        dir_entry *dir = (dir_entry * ) BLOCK(current_dir);
        int n_entradas=dir[0].size;

        int i, b, blocksize, remain_size;
        char *c;

        for(i=0; i<n_entradas; i++) {
                if(strcmp(dir[i].name, nome_fich)==0) {
                        entry=&dir[i];
                        break;
                }
        }
        b = entry->first_block;
        remain_size = entry->size;

        while( remain_size )
    {

        if( remain_size > sb->block_size )
                        blocksize = sb->block_size;
        else
                        blocksize = remain_size;

                        c = (char *)BLOCK( b );
                        for( i = 0; i < blocksize; i++ )
                                putchar( c[i] );

                        remain_size = remain_size - blocksize;
                        b = fat[b];
    }

    putchar('\n');

}


void remove_entry(dir_entry *entry, dir_entry *dest)
{
           int b, b_prev,n_entradas;
         unsigned int i;
         dir_entry *aux, *prev;
        for( b = dest->first_block; b != -1; b = fat[b] ) {
                aux = (dir_entry *)BLOCK( b );
                 n_entradas=aux[b].size;
                 b_prev=b;
                 prev=&aux[i];
                for( i = 0; i < n_entradas; i++ ) {
                        prev = &aux[i];
                        if( aux[i].type != TYPE_DIR && aux[i].type != TYPE_FILE ) {

                                //set_info(entry, prev->type, prev->name, prev->day, prev->month, prev->year, prev->size, prev->first_block);
                                entry->type=prev->type;
                                strcpy(entry->name,prev->name);
                                entry->day=prev->day;
                                entry->month=prev->month;
                                entry->year=prev->year;
                                entry->size=prev->size;
                                entry->first_block =prev->first_block;
                                prev->type = 'x';
                                if( i == 0 ) {
                                        fat[ b_prev ] = -1;
                                        fat[b] = sb->free_block; sb->free_block = b;
                                }
                                return;
                        }
                        prev = &aux[i];
                }
                b_prev = b;
        }

}

void copy_blocks(int size, int orig_block, int dest_block){

        char *orig = BLOCK(orig_block);
        char *dest = BLOCK(dest_block);

        while(size > 0){
                int aux = 0;
                while(aux < sb->block_size){
                        dest[aux] = orig[aux];
                        aux++;
                        size--;
                }
                orig_block = fat[orig_block];
                dest_block = find_free_block();
        }

        return;
}


// cp fich1 fich2 - copia o ficheiro fich1 para fich2
// cp fich dir - copia o ficheiro fich para o subdirectório dir
void vfs_cp(char *nome_orig, char *nome_dest) {

                dir_entry *dir = (dir_entry*)BLOCK(current_dir);
        int size = dir[0].size;

        int orig = -1;
        int dest = -1;

        int i;
        for(i=0; i<size; i++){
                if(!strcmp(nome_orig, dir[i].name))
                        orig = i;
                if(!strcmp(nome_dest, dir[i].name))
                        dest = i;
        }

        if(orig == -1){
                perror("O ficheiro de origem nao existe.\n");
                return;
        }

        if(dest == -1){
                int free = find_free_block();
                init_dir_entry(&dir[dir[0].size], TYPE_FILE, nome_dest, dir[orig].size ,free);
                copy_blocks(dir[orig].size, dir[orig].first_block, free);
                dir[0].size++;
                return;
        }

        if(dir[dest].type == TYPE_FILE){

                put_free_block(dir[dest].first_block);

                int free = find_free_block();
          init_dir_entry(&dir[dir[0].size], TYPE_FILE, nome_dest, dir[orig].size ,free);
                copy_blocks(dir[orig].size, dir[orig].first_block, free);

                return;
        }

        int file_size = dir[orig].size;
        int block_orig = dir[orig].first_block;

        int block = dir[dest].first_block;
        dir = (dir_entry*)BLOCK(block);
        size = dir[0].size;

        int aux = -1;

        for(i=0; i<size; i++){
                if(!strcmp(nome_orig, dir[i].name)){
                        aux = i;
                        break;
                }
        }

        if(aux == -1){
                aux = size;
                dir[0].size++;
        }
        else{
                put_free_block(dir[aux].first_block);
        }

        int free = find_free_block();
        init_dir_entry(&dir[aux], TYPE_FILE, nome_orig, file_size, free);
        copy_blocks(file_size, block_orig, free);

        return;
}




// mv fich1 fich2 - move o ficheiro fich1 para fich2
// mv fich dir - move o ficheiro fich para o subdirectório dir
void vfs_mv(char *nome_orig, char *nome_dest) {
        dir_entry *dir = (dir_entry*)BLOCK(current_dir);
        int size = dir[0].size;

        int orig = -1;
        int dest = -1;

        int i;
        for(i=0; i<size; i++){
                if(!strcmp(nome_orig, dir[i].name))
                        orig = i;
                if(!strcmp(nome_dest, dir[i].name))
                        dest = i;
        }

        if(orig == -1){
                perror("O ficheiro de origem nao existe.\n");
                return;
        }

        if(dest == -1){
                strcpy(dir[orig].name, nome_dest);
                return;
        }

        if(dir[dest].type == TYPE_FILE){
                strcpy(dir[orig].name, nome_dest);
                put_free_block(dir[dest].first_block);
                dir[dest] = dir[size-1];
                dir[0].size--;
                return;
        }

        int file_size = dir[orig].size;
        int day = dir[orig].day;
        int month = dir[orig].month;
        int year = dir[orig].year;
        int file_block = dir[orig].first_block;

        int block = dir[dest].first_block;

        dir[orig] = dir[size-1];
        dir[0].size--;

        dir = (dir_entry*)BLOCK(block);
        size = dir[0].size;

        for(i=0; i<size; i++){
                if(!strcmp(nome_orig, dir[i].name)){
                        put_free_block(dir[i].first_block);
                        dir[i] = dir[size-1];
                        dir[0].size--;
                        break;
                }
        }

        init_dir_entry(&dir[size], TYPE_FILE, nome_orig, file_size, file_block);
        dir[size].day = day;
        dir[size].month = month;
        dir[size].year = year;
        dir[0].size++;

        return;
}


// rm fich - remove o ficheiro fich
void vfs_rm(char *nome_fich) {
        dir_entry*  dir = (dir_entry*)BLOCK(current_dir);
  int size = dir[0].size;

  int i;
  for(i=0; i<size; i++){
    if(!strcmp(nome_fich, dir[i].name)){

      if(dir[i].type == TYPE_DIR){
        perror("Nao pode utilizar o comando rm para remover um diretorio.\n");
        return;
      }

      put_free_block(dir[i].first_block);
      dir[i] = dir[size-1];
      dir[0].size--;
      return;
    }
  }

  perror("O ficheiro nao existe.\n");

  return;
 }
