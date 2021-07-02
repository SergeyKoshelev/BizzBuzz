#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/msg.h>

const char* mes_gen_path = "/home/sergey/messages/mes_gen";

struct msgbuf 
{
    long mtype;
    pid_t pid;
};

const long forward = 1;
const long back = 2;

int main()
{
    key_t key = ftok(mes_gen_path, 1);    
    int id = msgget(key, IPC_CREAT | 0666);
    pid_t pid = getpid();

    struct msgbuf msg = {forward, pid};
    printf("my pid: %d\n", pid);
    msgsnd(id, &msg, sizeof(struct msgbuf), 0);

    int size = msgrcv(id, &msg, sizeof(struct msgbuf), back, 0);
    printf("pid from message: %d\n", msg.pid);
}