#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#define BUFSZ 256

#define LOG_PATH "programm.log"
#define ERR 0

//init log file (on LOG_PATH if path == NULL)
int log_init(char * path);

//create buffer of log and put in file
int log_error(int level, char* format, ...);

//clear buffer
void clear_buf(char* buffer, int size);

//write level in buffer
int insert_level(int level);

//write time in buffer
int insert_time();

//write pid in buffer
int insert_pid();

//macros
#define pr_err(format, ...)  log_error(ERR , "[%s:%d] " format, __FILE__, __LINE__, ##__VA_ARGS__);