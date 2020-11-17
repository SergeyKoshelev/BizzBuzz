#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

//- don't create new stack, if it exists (solve problem with another size)
//+ check destruction
//+ add semaphores (for race condition)
//+ contain stack count in all programs using sems

int timeout_flag = 0;
struct timespec timeout_time = {0, 0}; 

struct stack_t;

/* Attach (create if needed) shared memory stack to the process.
Returns stack_t* in case of success. Returns NULL on failure. */
struct stack_t* attach_stack(key_t key, int size);

/* Detaches existing stack from process. 
Operations on detached stack are not permitted since stack pointer becomes invalid. */
int detach_stack(struct stack_t* stack);

/* Marks stack to be destroed. Destruction are done after all detaches */ 
int mark_destruct(struct stack_t* stack);

/* Returns stack maximum size. */
int get_size(struct stack_t* stack);

/* Returns current stack size. */
int get_count(struct stack_t* stack);

/* Push val into stack. */
int push(struct stack_t* stack, void* val);

/* Pop val from stack into memory */
int pop(struct stack_t* stack, void** val);

//---------------------------------------------
/* Additional tasks */

/* Control timeout on push and pop operations in case stack is full or empty.
val == -1 Operations return immediatly, probably with errors.
val == 0  Operations wait infinitely.
val == 1  Operations wait timeout time.
*/
int set_wait(int val, struct timespec* timeout);

//definitions
struct stack_t
{
    int size;
    void** memory;
    int shmem_id;
    sem_t* count;
    sem_t* flag;
    sem_t* full;
    sem_t* empty;
};

struct stack_t* attach_stack(key_t key, int size)
{
    struct stack_t* stack = (struct stack_t*)malloc(sizeof(struct stack_t));
    assert(stack != NULL);

    int id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
    void* memory = NULL; 

    //printf("id = %d\n", id);
    if (id < 0)  //already created
    {
        memory = shmat(id, NULL, 0);
        assert(memory != NULL);

        stack->count = sem_open("count", O_RDWR);
        stack->flag = sem_open("flag", O_RDWR);
        stack->full = sem_open("full", O_RDWR);
        stack->empty = sem_open("empty", O_RDWR);
    }
    else  //create new 
    {
        //printf("im here\n");
        int id = shmget(key, size, IPC_CREAT | 0666);
        memory = shmat(id, NULL, 0);
        assert(memory != NULL);

        stack->count = sem_open("count", O_CREAT | O_RDWR, 0666, 0);
        stack->flag = sem_open("flag", O_CREAT | O_RDWR, 0666, 0);
        stack->full = sem_open("full", O_CREAT | O_RDWR, 0666, 0);
        stack->empty = sem_open("empty", O_CREAT | O_RDWR, 0666, 0);
        sem_post(stack->flag);
        if (size != 0)
            sem_post(stack->full);
    }
    
    if ((memory == NULL) || (stack == NULL) || (stack->count == NULL) || (stack->flag == NULL) || (stack->full == NULL) || (stack->empty == NULL))
    {
        printf("Error with init stack\n");
        return NULL;
    }

    stack->size = size;
    stack->memory = memory;
    stack->shmem_id = id;
    return stack;
}

int detach_stack(struct stack_t* stack)
{
    if (stack == NULL)
    {
        printf("Invalid stack pointer in push\n");
        return -1;
    }
    else
    {
        int check = shmdt(stack->memory);
        assert (check >= 0);
        check = sem_close(stack->count);
        assert(check >= 0);
        check = sem_close(stack->flag);
        assert(check >= 0);
        check = sem_close(stack->full);
        assert(check >= 0);
        check = sem_close(stack->empty);
        assert(check >= 0);
    }
}

int mark_destruct(struct stack_t* stack)
{
    if (stack == NULL)
    {
        printf("Invalid stack pointer in destruction\n");
        return -1;
    }
    else 
    {
        int check = shmctl(stack->shmem_id, IPC_RMID, NULL);
        check = sem_unlink("count");
        check = sem_unlink("flag");
        check = sem_unlink("full");
        check = sem_unlink("empty");
    }
}

int get_size(struct stack_t* stack)
{
    if (stack == NULL)
        return -1;
    else
        return stack->size;
}

int get_count(struct stack_t* stack)
{
    int cur_count = 0;
    if (stack == NULL)
        return -1;
    else
    {
        int check = sem_getvalue(stack->count, &cur_count);
        assert(check >= 0);
        return cur_count;
    }
}

int push(struct stack_t* stack, void* val)
{
    if (stack == NULL)
    {
        printf("Invalid stack pointer in push\n");
        return -1;
    }

    if (timeout_flag == -1) //immediately
    {
        sem_wait(stack->flag);
        if (get_count(stack) == get_size(stack))
        {
            printf("Cant push in full stack\n");
            sem_post(stack->flag);
            return -1;
        }
        else 
        {
            memcpy(stack->memory + get_count(stack), &val, sizeof(val));
            sem_post(stack->count);
            if (get_count(stack) == get_size(stack))
                sem_wait(stack->full);
            if (get_count(stack) == 1)
                sem_post(stack->empty);
            sem_post(stack->flag);
            return 0;
        }
    }
    else if (timeout_flag == 0) //wait inifinitely
    {
        int value;
        sem_wait(stack->full);
        printf("im here 1\n");
        sem_wait(stack->flag);
        printf("%d\n", get_count(stack));
        memcpy(stack->memory + get_count(stack), &val, sizeof(val));
        printf("im here 3\n");
        sem_post(stack->count);
        if (get_count(stack) < get_size(stack))
            sem_post(stack->full);
        sem_getvalue(stack->full, &value);
        printf("%d\n", value);
        if (get_count(stack) == 1)
            sem_post(stack->empty);
        sem_post(stack->flag);
        return 0;
    }
    else if (timeout_flag == 1) //wait only definite time
    {   
        //under construction
        return -1;
    }
    else
    {
        printf("unknown timeout flag\n");
        sem_post(stack->flag);
        return -1;
    }
    
}

int pop(struct stack_t* stack, void** val)
{
    if (stack == NULL)
    {
        printf("Invalid stack pointer in push\n");
        return -1;
    }

    
    if (timeout_flag == -1)   //immediately
    {
        sem_wait(stack->flag);
        if (get_count(stack) == 0)
        {
            printf("Cant pull from empty stack\n");
            sem_post(stack->flag);
            return -1;
        }
        else 
        {   
            memcpy(val, stack->memory + get_count(stack) - 1, sizeof(*val));
            sem_wait(stack->count);
            if (get_count(stack) == get_size(stack) - 1)
                sem_post(stack->full);
            if (get_count(stack) == 0)
                sem_wait(stack->empty);
            sem_post(stack->flag);
            return 0;
        }
    }
    else if (timeout_flag == 0) //wait infinitely
    {
        sem_wait(stack->empty);
        sem_wait(stack->flag);

        sem_wait(stack->count);
        memcpy(val, stack->memory + get_count(stack), sizeof(*val));
        if (get_count(stack) == get_size(stack) - 1)
                sem_post(stack->full);
        if (get_count(stack) > 0)
                sem_post(stack->empty);

        sem_post(stack->flag);
        return 0;
    }
    else if (timeout_flag == 1) //wait only definite time
    {
        //under construction
        int value = 0;
        sem_getvalue(stack->empty, &value);
        printf("value = %d\n", value);
        printf("time s = %ld\n", timeout_time.tv_sec);
        sem_timedwait(stack->empty, &timeout_time);
        sem_getvalue(stack->empty, &value);
        printf("value = %d\n", value);
        printf("im here\n");
        sem_wait(stack->flag);

        sem_wait(stack->count);
        memcpy(val, stack->memory + get_count(stack), sizeof(*val));
        if (get_count(stack) == get_size(stack) - 1)
                sem_post(stack->full);
        if (get_count(stack) > 0)
                sem_post(stack->empty);

        sem_post(stack->flag);
        return -1;
    }
    else
    {
        printf("unknown timeout flag\n");
        sem_post(stack->flag);
        return 0;
    }
    
}

int set_wait(int val, struct timespec* timeout)
{
    timeout_flag = val;
    if (timeout != NULL)
    {
        timeout_time.tv_nsec = timeout->tv_nsec;
        timeout_time.tv_sec = timeout->tv_sec;
    }
}