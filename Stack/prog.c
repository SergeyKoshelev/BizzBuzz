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

//simplest program with 1 process
//it push size+1 elems and than pop size+1 elems

const int size = 30;

int main(int argc, char** argv)
{
    key_t key = atoi(argv[1]);

    clear_key(key, size);

    struct stack_t* stack = attach_stack(key, size);
    //printf("%p\n", stack);
    void * a = NULL;
    int check;
    
    for (int i = 0; i < size + 1; i++)
    {
        a += 1;
        check = push (stack, a);
        if (check == 0) 
            printf("pushed %p\n", a);
    }
    
    for (int i = 0; i < size + 1; i++)
    {
        check = pop (stack, &a);
        if (check == 0) 
            printf("popped %p\n", a);
    }

    detach_stack(stack);
    mark_destruct(stack);
    free(stack);
}