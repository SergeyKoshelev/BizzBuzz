#include "stack.h"

const char* s_count = "count";
const char* s_flag = "flag";

int timeout_flag = 0;
struct timespec timeout_time = {0, 0}; 

char* key_count;
char* key_flag;

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

    key_count = (char*)malloc(strlen(s_count) + 10);
    key_flag = (char*)malloc(strlen(s_flag) + 10);
    assert ((key_count != NULL) && (key_flag != NULL));
    sprintf(key_count, "%s%d", s_count, key);
    sprintf(key_flag, "%s%d", s_flag, key);

    void* memory = NULL; 
    int id = shmget(key, size, IPC_EXCL | 0666);
    int my_errno = errno;
    
    if (id > 0)  //already created
    {
        //printf("dont create, attach with created\n");
        memory = shmat(id, NULL, 0);
        assert(memory != NULL);

        stack->count = sem_open(key_count, O_RDWR);
        if ((errno == ENOENT) && (stack->count == NULL))
        {
            printf("can't attach to existing semaphore count\n");
            free(stack);
            return NULL;
        }
        stack->flag = sem_open(key_flag, O_EXCL | 0666);
        if ((errno == ENOENT) && (stack->flag == NULL))
        {
            printf("can't attach to existing semaphore flag\n");
            free(stack);
            return NULL;
        }
    }
    else if (my_errno == EINVAL) //bad size
    {
        printf("can't create so big stack, because was created smaller one\n");
        free(stack);
        return NULL;
    }
    else  //create new 
    {
        //printf("creating new stack\n");
        id = shmget(key, size, IPC_CREAT | 0666);
        assert(id > 0);
        memory = shmat(id, NULL, 0);
        assert(memory != NULL);
        //printf("new memory %p\n", memory);

        //printf("key_count = %s\n", key_count);
        stack->count = sem_open(key_count, O_CREAT | O_RDWR, 0666, 0);
        if (errno == EEXIST)
        {
            printf("can't create new semaphore count\n");
            free(stack);
            return NULL;
        }

        stack->flag = sem_open(key_flag, O_CREAT | O_EXCL, 0666, 0);
        if (errno == EEXIST)
        {
            printf("can't create new semaphore flag\n");
            free(stack);
            return NULL;
        }
        
        assert ((stack->flag != NULL) && (stack->count != NULL));
        sem_post(stack->flag);
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
        //printf("destructing sems and shmem\n");
        int check = shmctl(stack->shmem_id, IPC_RMID, NULL);
        assert(check == 0);
        check = sem_unlink(key_count);
        assert(check == 0);
        check = sem_unlink(key_flag);
        assert(check == 0);
        free(key_count);
        free(key_flag);
        return 0;
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

    if (timeout_flag == 0) //wait infinitely
        sem_wait(stack->flag);
    else if (timeout_flag == -1) //try immediately
    {
        sem_trywait(stack->flag);
        if (errno == EAGAIN)
        {
            //printf("cant't trywait\n");
            return -1;            }
        }
    else if (timeout_flag == -1) //wait time
    {
        sem_timedwait(stack->flag, &timeout_time);
        if (errno == ETIMEDOUT)
        {
            //printf("cant't timedwait\n");
            return -1;
        }
    }

    //printf("count = %d  size = %d\n", get_count(stack), get_size(stack));
    if (get_count(stack) >= get_size(stack))
    {
        printf("stack is full (size is %d), can't push\n", get_size(stack));
        sem_post(stack->flag);
        return -1;
    }
    
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

    if (timeout_flag == 0) //wait infinitely
        sem_wait(stack->flag);
    else if (timeout_flag == -1) //try immediately
    {
        sem_trywait(stack->flag);
        if (errno == EAGAIN)
        {
            //printf("cant't trywait\n");
            return -1;
        }
    }
    else if (timeout_flag == -1) //wait time
    {
        sem_timedwait(stack->flag, &timeout_time);
        if (errno == ETIMEDOUT)
        {
            //printf("cant't timedwait\n");
            return -1;
        }
    }

    if (get_count(stack) == 0)
    {
        printf("stack is empty, can't pop\n");
        sem_post(stack->flag);
        return -1;
    }
    
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

key_t rand_key_gen(int argc, char** argv)
{
    if (argc == 1)
    {
        srand(time(NULL));
        return rand(); 
    }
    else
        return atoi(argv[1]);
}