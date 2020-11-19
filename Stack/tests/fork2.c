#include "stack.h"

//using constants push_count, fork_count and size create your own test
//every process do push_count pushes and pops in stack

const int size = 1000000;
const char* path = "/home/sergey/stack/gen";
const int push_count = 10;
const int fork_count = 11;

int main(int argc, char** argv)
{
    key_t key = rand_key_gen (argc, argv);
    
    pid_t main_pid = getpid();
    //printf("main pid: %d\n", main_pid);

    clear_key(key, size);
    printf("key is clear\n");
    struct stack_t* stack = attach_stack(key, size);
    pid_t pid;

    setgid(main_pid);

    for (int i = 0; i < fork_count; i++)
    {
        pid = fork();
    }

    if (getpid() != main_pid)
        stack = attach_stack(key, size);
    
    assert(stack != NULL);

    for (int j = 0; j < push_count; j++)
        {
            void* a = NULL + j + pid * 5;
            int check = push(stack, a);
            //if (check == 0) { printf("pushed %p by pid %d\n", a, getpid());}    
        }
        for (int j = 0; j < push_count; j++)
        {
            void* a;
            int check = pop(stack, &a);
            //if (check == 0) { printf("popped %p by pid %d\n", a, getpid()); }    
        }
    //printf("stack count: %d\n", get_count(stack));


    if (getpid() == main_pid)
    {
        void* a;
        setgid(main_pid + 1);
        killpg(main_pid, SIGKILL);
        push(stack, NULL);
        push(stack, NULL);
        pop(stack, &a);
        pop(stack, &a);
        printf("stack count in the end: %d\n", get_count(stack));
        detach_stack(stack);
        mark_destruct(stack);
    }
    else
    {
        //printf("end of the child with pid %d\n", getpid());
        detach_stack(stack);
    }
}