#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <assert.h>
#include "stack.h"
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

//prog with some 2 processes
//each process push size/2 elems and then pop size/2 elems

const int size = 100;
const char* path = "/home/sergey/stack/stack.txt";

int main(int argc, char** argv)
{
    key_t key;
    if (argc == 1)
    {
        key_t key = ftok(path, 100);    
    }
    else
    {
        key = atoi(argv[1]);
    }

    clear_key(key, size);
    
    pid_t pid = fork();
    struct stack_t* stack = attach_stack(key, size);
    assert(stack != NULL);
    
    for (int i = 0; i < size/2; i++)
    {
        void* a = NULL + i + pid * 5;
        int check = push(stack, a);
        if (check == 0) 
            printf("pushed: %p\n", a);    
    }
    for (int i = 0; i < size/2; i++)
    {
        void* a;
        int check = pop(stack, &a);
        if (check == 0) 
            printf("popped: %p\n", a);    
    }

    if (pid != 0)
    {
        sleep(1);
        printf("stack count in the end: %d\n", get_count(stack));
        detach_stack(stack);
        mark_destruct(stack);
    }
    else
    {
        detach_stack(stack);
    }
    
}