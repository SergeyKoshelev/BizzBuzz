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
    int id = msgget(key, 0);
    struct msgbuf msg;

    int size = msgrcv(id, &msg, sizeof(struct msgbuf), forward, 0);
    printf("pid from message: %d\n", msg.pid);

    pid_t pid = getpid();
    msg.pid = pid;
    msg.mtype = back;
    printf("my pid: %d\n", pid);
    msgsnd(id, &msg, sizeof(struct msgbuf), 0);
}