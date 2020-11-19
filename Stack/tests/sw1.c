#include "stack.h"

//simplest program with 1 process
//it push size elems and than pop size+1 elems

const int size = 10;

int main(int argc, char** argv)
{
    key_t key = rand_key_gen (argc, argv);

    clear_key(key, size);
    pid_t pid = fork();

    struct stack_t* stack = attach_stack(key, size);
    assert(stack != NULL);

    struct timespec timeout;
    timeout.tv_nsec = 0;
    timeout.tv_sec = 2;
    set_wait(1, &timeout);

    void * a = NULL;
    int check;

    for (int i = 0; i < size; i++)
    {
        a += 1;
        check = push (stack, a);
        //if (check == 0) {printf("pushed %p by pid %d\n", a, getpid());}
        //printf("stack count: %d    pid: %d\n", get_count(stack), getpid());
    }
    
    for (int i = 0; i < size; i++)
    {
        check = pop (stack, &a);
        //if (check == 0) { printf("popped %p\n", a); }
        //printf("stack count: %d    pid: %d\n", get_count(stack), getpid());
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
        //printf("end of the child\n");
        detach_stack(stack);
    }
    
    free(stack);
}