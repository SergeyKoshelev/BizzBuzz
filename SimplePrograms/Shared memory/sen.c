#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>

const char* mem_gen_path = "/home/sergey/shared_mem/mem_gen";
const char* size_gen_path = "/home/sergey/shared_mem/size_gen";

int main (int argc, char** argv) 
{
    int fd = open(argv[1], O_RDONLY);
    struct stat fd_stat;
    fstat(fd, &fd_stat);
    int fd_size = fd_stat.st_size;
    printf("file size: %d\n", fd_size);

    char* buf = (char*) calloc(fd_size, sizeof(char));
    read(fd, buf, fd_size);
    
    key_t key = ftok(mem_gen_path, 1);    
    int id = shmget(key, fd_size * sizeof(char), IPC_CREAT | 0666);
    void* memory = shmat(id, NULL, 0);
    //printf("im here\n");
    //printf("%p\n", memory);
    memcpy(memory, buf, fd_size);

    key_t key_s = ftok(size_gen_path, 1); 
    int id_s = shmget(key_s, sizeof(int), IPC_CREAT | 0666);
    void* sh_size = shmat(id_s, NULL, 0);
    memcpy(sh_size, &fd_size, sizeof(int));

    shmdt(memory);
    shmdt(sh_size);
    free(buf);
    close(fd);
}
