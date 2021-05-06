#include "log.h"
static char buf_log[LOGBUFSZ];
static int logfile;

int log_init(char * path)
{
    if (path == NULL)
        logfile = STDOUT_FILENO;
    else
        logfile = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);

    if (logfile < 0) {
        perror("opening log file");
        return -1;
    }
    return 0;
}

int logg(int level, char* fmt, ...)
{
    memset(buf_log, 0, LOGBUFSZ);

    va_list params;
    va_start(params, fmt);
    insert_level(level);
    insert_time();
    insert_pid();
    if (vsnprintf(buf_log + strlen(buf_log), LOGBUFSZ - strlen(buf_log) - 1, fmt, params) < 0) {
        perror("vsnprintf in lor_error");
        return -1;
    }
    buf_log[strlen(buf_log)] = '\n';

    if (write(logfile, buf_log, strlen(buf_log)) < 0) {
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
        count = snprintf(buf_log + strlen(buf_log), LOGBUFSZ - strlen(buf_log), "[ERR]");
        break;
    case INFO :
        count = snprintf(buf_log + strlen(buf_log), LOGBUFSZ - strlen(buf_log), "[INFO]");
        break;
    default :
        perror("unknown level");
        return -1;
    }

    if (count < 0) {
        perror("snprintf in write_level");
        return -1;
    }

    //printf("str: (%s)\n", buf_log);
    return 0;
}

int insert_time()
{
    time_t timer;
    time(&timer);
    char * strtime = ctime(&timer);
    strtime[strlen(strtime) - 1] = '\0';  //replace \n with \0

    int count = snprintf(buf_log + strlen(buf_log), LOGBUFSZ - strlen(buf_log), "[%s]", strtime);
    if (count < 0) {
        perror("snprintf in insert_time");
        return -1;
    }

    //printf("str: (%s)\n", buf_log);
    return 0;
}

int insert_pid()
{
    int count = snprintf(buf_log + strlen(buf_log), LOGBUFSZ - strlen(buf_log), "[%d]", getpid());
    if (count < 0) {
        perror("snprintf in insert_pid");
        return -1;
    }
    return 0;
}