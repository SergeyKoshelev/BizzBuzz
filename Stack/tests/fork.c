#include "stack.h"

//program with 2 processes
//the first push size+1 elems, than the second pop size+1 elems

const int size = 20;

int main(int argc, char** argv)
{
    key_t key = rand_key_gen(argc, argv);
    pid_t main_pid = getpid();
    pid_t pid = fork();

    struct stack_t* stack = attach_stack(key, size);

    void * a = NULL + pid;
    int check;
    
    if (pid != 0)
    {
        for (int i = 0; i < size; i++)
        {
            check = push (stack, a + i);
            //if (check == 0) printf("pushed in parent %p\n", a + i);
        }
        sleep(1);
    }
    
    if (pid == 0)
    {
        for (int i = 0; i < size; i++)
        {
            check = pop (stack, &a);
            //if (check == 0) printf("popped in child %p\n", a);
        }
    }
    if (getpid() == main_pid)
    {
        sleep(2);
        printf("stack count in the end: %d\n", get_count(stack));
        detach_stack(stack);
        mark_destruct(stack);
    }
    else
    {
        //printf("end of the child with pid %d\n", getpid());
        detach_stack(stack);
    }
    free(stack);
}