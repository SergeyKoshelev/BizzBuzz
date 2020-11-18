#include "stack.h"

//simplest program with 1 process
//it push size+1 elems and than pop size+1 elems

const int size = 100;

int main(int argc, char** argv)
{
    set_wait(-1, NULL);
    key_t key = atoi(argv[1]);

    clear_key(key, size);
    pid_t pid = fork();

    struct stack_t* stack = attach_stack(key, size);
    //printf("pid: %d\n", getpid());
    //printf("%p\n", stack);
    void * a = NULL;
    int check;
    
    for (int i = 0; i < size; i++)
    {
        a += 1;
        check = push (stack, a);
        //if (check == 0) {printf("pushed %p\n", a);}
    }
    
    for (int i = 0; i < size; i++)
    {
        check = pop (stack, &a);
        //if (check == 0) { printf("popped %p\n", a); }
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