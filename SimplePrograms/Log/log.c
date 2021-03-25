#include "log.h"
static char buffer[BUFSZ];
static int logfile;

int log_init(char * path)
{
    if (path == NULL)
        logfile = open(LOG_PATH, O_WRONLY | O_CREAT | O_APPEND, 0666);
    else
        logfile = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);

    if (logfile < 0) {
        perror("opening log file");
        return -1;
    }
    return 0;
}

void clear_buf(char* buffer, int size)
{
    for (int i = 0; i < size; i++)
        buffer[i] = 0;
}

int log_error(int level, char* fmt, ...)
{
    assert(logfile >= 0);
    clear_buf(buffer, BUFSZ);

    va_list params;
    va_start(params, fmt);
    insert_level(level);
    insert_time();
    insert_pid();
    if (vsnprintf(buffer + strlen(buffer), BUFSZ - strlen(buffer) - 1, fmt, params) < 0) {
        perror("vsnprintf in lor_error");
        return -1;
    }
    buffer[strlen(buffer)] = '\n';

    if (write(logfile, buffer, BUFSZ) < 0) {
        perror("writing in logfile");
        return -1;
    }

    return 0;
}

int insert_level(int level)
{
    int count = 0;

    switch (level) {
    case ERR :
        count = snprintf(buffer + strlen(buffer), BUFSZ - strlen(buffer), "[ERR]");
        break;
    default :
        perror("unknown level");
        return -1;
    }

    if (count < 0) {
        perror("snprintf in write_level");
        return -1;
    }

    //printf("str: (%s)\n", buffer);
    return 0;
}

int insert_time()
{
    time_t timer;
    time(&timer);
    char * strtime = ctime(&timer);
    strtime[strlen(strtime) - 1] = '\0';  //replace \n with \0

    int count = snprintf(buffer + strlen(buffer), BUFSZ - strlen(buffer), "[%s]", strtime);
    if (count < 0) {
        perror("snprintf in insert_time");
        return -1;
    }

    //printf("str: (%s)\n", buffer);
    return 0;
}

int insert_pid()
{
    int count = snprintf(buffer + strlen(buffer), BUFSZ - strlen(buffer), "[%d]", getpid());
    if (count < 0) {
        perror("snprintf in insert_pid");
        return -1;
    }
    return 0;
}