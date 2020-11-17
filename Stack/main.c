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

//using constants push_count, fork_count and size create your own test
//every process do push_count pushes and pops in stack

const int size = 100000;
const char* path = "/home/sergey/stack/gen";
const int push_count = 20;
const int fork_count = 10;

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
    pid_t main_pid = getpid();

    struct stack_t* stack;
    pid_t pid;

    for (int i = 0; i < fork_count; i++)
    {
        pid = fork();
        stack = attach_stack(key, size);
        assert(stack != NULL);
    
        for (int i = 0; i < push_count; i++)
        {
            void* a = NULL + i + pid * 5;
            int check = push(stack, a);
            if (check == 0) 
            {
                //printf("pushed: %p\n", a);
            }    
        }
        for (int i = 0; i < push_count; i++)
        {
            void* a;
            int check = pop(stack, &a);
            if (check == 0)
            { 
                //printf("popped: %p\n", a);
            }    
        }
        //printf("stack count: %d\n", get_count(stack));
    }


    if (getpid() == main_pid)
    {
        sleep(4);
        printf("stack count in the end: %d\n", get_count(stack));
        detach_stack(stack);
        mark_destruct(stack);
    }
    else
    {
        //printf("end of the child\n");
        detach_stack(stack);
    }
}