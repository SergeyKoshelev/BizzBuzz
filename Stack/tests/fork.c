#include "stack.h"

//program with 2 processes
//the first push size+1 elems, than the second pop size+1 elems

const int size = 30;

int main(int argc, char** argv)
{
    key_t key = atoi(argv[1]);
    pid_t pid = fork();

    struct stack_t* stack = attach_stack(key, size);

    void * a = NULL + pid;
    int check;
    
    if (pid != 0)
    {
        for (int i = 0; i < size + 1; i++)
        {
            check = push (stack, a + i);
            if (check == 0) 
                printf("pushed in parent %p\n", a + i);
        }
    }

    sleep(1);
    if (pid == 0)
    {
        for (int i = 0; i < size + 1; i++)
        {
            check = pop (stack, &a);
            if (check == 0) 
                printf("popped in child %p\n", a);
        }
    }

    detach_stack(stack);
    if (pid != 0)
    {
        sleep(1);
        mark_destruct(stack);
    }
    free(stack);
}