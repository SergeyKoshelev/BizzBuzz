#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

int timeout_flag = -1;
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

/* Clear fake pieces of stack from previous stacks with this key */
int clear_key (key_t key, int size);

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
};

void print_sem(const char* name, sem_t* sem)
{
    int val;
    sem_getvalue(sem, &val);
    printf("semaphore %s = %d\n", name, val);
}

struct stack_t* attach_stack(key_t key, int size)
{
    struct stack_t* stack = (struct stack_t*)calloc(1, sizeof(struct stack_t));
    assert(stack != NULL);

    int id = shmget(key, size, IPC_EXCL | 0666);
    void* memory = NULL; 

    if (id > 0)  //already created
    {
        //printf("dont create, attach with created\n");
        memory = shmat(id, NULL, 0);
        //printf("old memory %p\n", memory);

        stack->count = sem_open("count", O_RDWR);
        stack->flag = sem_open("flag", O_RDWR);
    }
    else  //create new 
    {
        //printf("creating new stack\n");
        id = shmget(key, size, IPC_CREAT | 0666);
        assert(id > 0);
        memory = shmat(id, NULL, 0);
        //printf("new memory %p\n", memory);

        stack->count = sem_open("count", O_CREAT | O_RDWR, 0666, 0);
        stack->flag = sem_open("flag", O_CREAT | O_RDWR, 0666, 0);
        sem_post(stack->flag);
    }
    
    if ((memory == NULL) || (stack == NULL) || (stack->count == NULL) || (stack->flag == NULL))
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
    assert (timeout_flag == -1); //do immidiately

    if (get_count(stack) == get_size(stack))
    {
        printf("stack is full (size is %d), can't push\n", get_size(stack));
        return -1;
    }

    sem_wait(stack->flag);
    stack->memory[get_count(stack)] = val;
    sem_post(stack->count);
    sem_post(stack->flag);
    return 0;
}

int pop(struct stack_t* stack, void** val)
{
    
    if (stack == NULL)
    {
        printf("Invalid stack pointer in push\n");
        return -1;
    }

    assert(timeout_flag == -1); //do immidiately

    if (get_count(stack) == 0)
    {
        printf("stack is empty, can't pop\n");
        return -1;
    }

    sem_wait(stack->flag);
    sem_wait(stack->count);
    *val = stack->memory[get_count(stack)]; 
    sem_post(stack->flag);
    return 0;
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

int clear_key (key_t key, int size)
{
    struct stack_t* stack = attach_stack(key, size);
    assert((stack != NULL) && "Null pointer in clear_key");
    mark_destruct(stack);
}