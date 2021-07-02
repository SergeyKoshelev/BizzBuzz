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
    int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    
    key_t key = ftok(mem_gen_path, 1);    
    key_t key_s = ftok(size_gen_path, 1);    

    int size = 0;
    int id_s = shmget(key_s, sizeof(int), IPC_CREAT | 0666);
    void* sh_size = shmat(id_s, NULL, 0); 
    memcpy(&size, sh_size, sizeof(int));

    int id = shmget(key, size * sizeof(char), IPC_CREAT | 0666);
    void* memory = shmat(id, NULL, 0); 
    //printf("%p\n", memory);
    char* buf = (char*) calloc(size, sizeof(char));
    memcpy(buf, memory, size);

    write(fd, buf, size);

    shmdt(sh_size);
    shmdt(memory);
    shmctl(id, IPC_RMID, NULL);
    shmctl(id_s, IPC_RMID, NULL);
    free(buf);
    close(fd);
}
