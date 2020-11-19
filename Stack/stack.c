#include "stack.h"

int timeout_flag = 0;
struct timespec timeout_time = {0, 0};

const int SEM_COUNT = 0;
const int SEM_FLAG = 1;

struct stack_t
{
    int size;
    void** memory;
    int shmem_id;
    int sem_id; //count: semnum == 0    flag: semnum == 1
};

void print_sem(int sem_id, int sem_num)
{
    int val = semctl(sem_id, sem_num, GETVAL);
    if (val == -1)
        printf("Invalid semaphore in print_sem\n");
    else
        printf("value of %d sem: %d\n", sem_num, val);
    
}

struct stack_t* attach_stack(key_t key, int size)
{
    struct stack_t* stack = (struct stack_t*)calloc(1, sizeof(struct stack_t));
    assert(stack != NULL);

    void* memory = NULL; 
    int shmem_id = shmget(key, size, IPC_EXCL | 0666);
    int my_errno = errno;
    int sem_id;
    
    if (shmem_id > 0)  //already created
    {
        //printf("dont create, attach with created\n");
        memory = shmat(shmem_id, NULL, 0);
        assert(memory != NULL);

        sem_id = semget(key, 2, IPC_EXCL | 0666);
        if (sem_id <= 0) // errno == ENOENT
        {
            printf("can't attach to existing semaphore count\n");
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
        shmem_id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
        assert(shmem_id > 0);
        memory = shmat(shmem_id, NULL, 0);
        assert(memory != NULL);

        sem_id = semget(key, 2, IPC_CREAT | IPC_EXCL | 0666);
        //my_errno = errno;
        //printf("%d %d\n", my_errno, EEXIST);
        if ((errno == EEXIST)|| (sem_id <= 0))
        {
            printf("can't create new semaphore count\n");
            free(stack);
            return NULL;
        }
        
        assert (sem_id > 0);
        sem_change(sem_id, SEM_FLAG, 1);
    }

    stack->size = size;
    stack->memory = memory;
    stack->shmem_id = shmem_id;
    stack->sem_id = sem_id;
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
        check = semctl(stack->sem_id, 0, IPC_RMID);
        assert(check == 0);
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
    if (stack == NULL)
        return -1;
    else
    {
        int val = semctl(stack->sem_id, SEM_COUNT, GETVAL);
        if (errno == EIDRM)
        {
            printf("finished because semaphores were removed\n");
            exit(1);
        }
        return val;
    }
}

int push(struct stack_t* stack, void* val)
{
    if (stack == NULL)
    {
        printf("Invalid stack pointer in push\n");
        return -1;
    }

    int check = sem_change(stack->sem_id, SEM_FLAG, -1);
    if (check == 0)
    {
        if (get_count(stack) >= get_size(stack))
        {
            printf("stack is full (size is %d), can't push\n", get_size(stack));
            sem_change(stack->sem_id, SEM_FLAG, 1);
            return -1;
        }
    
        stack->memory[get_count(stack)] = val;
        sem_change(stack->sem_id, SEM_COUNT, 1);
        sem_change(stack->sem_id, SEM_FLAG, 1);
        return 0;
    }
    return check;
}

int pop(struct stack_t* stack, void** val)
{
    if (stack == NULL)
    {
        printf("Invalid stack pointer in push\n");
        return -1;
    }

    int check = sem_change(stack->sem_id, SEM_FLAG, -1);
    if (check == 0)
    {
        if (get_count(stack) == 0)
        {
            printf("stack is empty, can't pop\n");
            sem_change(stack->sem_id, SEM_FLAG, 1);
            return -1;
        }

        *val = stack->memory[get_count(stack) - 1];
        sem_change(stack->sem_id, SEM_COUNT, -1);
        sem_change(stack->sem_id, SEM_FLAG, 1);
        return 0; 
    }

    return check;    
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

int sem_change(int sem_id, int sem_num, int val)
{
    int res;
    struct sembuf sems;
    sems.sem_num = sem_num;
    sems.sem_op = val;

    if (sem_num == SEM_COUNT) //change count
    {
        sems.sem_flg = 0;
        return semop(sem_id, &sems, 1);
    }

    
    else if (sem_num == SEM_FLAG) //change flag
    {
        if (timeout_flag == -1) //no wait, do immediately
        {
            sems.sem_flg = IPC_NOWAIT | SEM_UNDO;
            res = semop(sem_id, &sems, 1);
            //if (res == -1) /*if (errno == EAGAIN)*/ printf("So fast, sem is not ready\n");
            return res; 
        }
        else if (timeout_flag == 0) //wait inifinity time
        {
            sems.sem_flg = SEM_UNDO;
            res = semop(sem_id, &sems, 1);
            //if (res == -1) printf("ERROR sem_change 0\n");
            return res; 
        }
        else if (timeout_flag == 1) //wait only timeout time
        {
            sems.sem_flg = SEM_UNDO;
            res = semtimedop(sem_id, &sems, 1, &timeout_time);
            if (res == -1) /* if (errno == EAGAIN) */ printf("Not enough time for sem_change\n");
            return res; 
        } 
        else 
        {
            printf("Invalid timeout_flag in sem_change\n");
            return -1;
        }
    }
    else
    {
        printf("Invalid sem_num in sem_change()\n");
        return -1;
    }
}