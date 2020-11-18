#include "stack.h"

//program with 2 processes
//processes have different sizes in attach

const int size = 10;

int main(int argc, char** argv)
{
    key_t key = atoi(argv[1]);
    pid_t pid = fork();

    struct stack_t* stack;
    void * a = NULL + pid;
    int check;
    
    if (pid != 0)
    {
        sleep(1);
        stack = attach_stack(key, size);
        assert(stack != NULL);
        for (int i = 0; i < size; i++)
        {
            check = push (stack, a + i);
            //if (check == 0) printf("pushed in parent %p\n", a + i);
        }
    }

    if (pid == 0)
    {
        stack = attach_stack(key, size * 2);
        assert(stack != NULL);
        for (int i = 0; i < size * 2; i++)
        {
            check = push (stack, &a);
            //if (check == 0) printf("popped in child %p\n", a);
        }
    }

    sleep(2);
    if (pid != 0)
    {
        sleep(1);
        printf("stack count in the end: %d\n", get_count(stack));
        detach_stack(stack);
        mark_destruct(stack);
    }
    else
    {
        //printf("end of the child\n");
        detach_stack(stack);
    }
    
    free(stack);
}