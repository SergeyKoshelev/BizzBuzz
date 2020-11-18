#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <assert.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

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

/* Generate random key */
key_t rand_key_gen(int argc, char** argv);

//---------------------------------------------
/* Additional tasks */

/* Control timeout on push and pop operations in case stack is full or empty.
val == -1 Operations return immediatly, probably with errors.
val == 0  Operations wait infinitely.
val == 1  Operations wait timeout time.
*/
int set_wait(int val, struct timespec* timeout);