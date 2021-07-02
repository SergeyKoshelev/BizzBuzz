#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define BUF_SIZE 1024 //size for copying buf
#define COMMAND_SIZE 128
char buf[COMMAND_SIZE];
#define PATH_SIZE 128
int SIG_STOP = SIGINT;
const char my_fifo[] = "fifo_log"; 
const char log_file[] = "log.txt";
int fifo_fd = -1;

void create_fifo()
{
    int check_mk = mkfifo(my_fifo, S_IRUSR | S_IWUSR);
    if ((check_mk == -1) && (errno != EEXIST))
    {
        printf("Can't create fifo %s for logging\n", my_fifo);
        exit(1);
    }

    fifo_fd = open(my_fifo, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) 
    {
        printf("Can't open fifo %s for logging\n", my_fifo);
        exit(1);
    }
}

void start(char* src, char* dest, char* behaviour)
{
    pid_t tmp_pid;
    tmp_pid = fork();
    if (tmp_pid == 0)
        execl("backuper", "backuper", src, dest, behaviour, NULL);
}

void stop(pid_t pid)
{
    kill(pid, SIG_STOP);
}

void change (pid_t pid, char* src, char* dest, char* behaviour)
{
    stop(pid);
    start(src, dest, behaviour);
}

void write_log()
{
    int logtxt_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (logtxt_fd == -1) 
    {
        printf("Can't open logfile %s for logging\n", log_file);
        exit(1);
    }
    char buf[BUF_SIZE];
    int count; 
    while ((count = read(fifo_fd, buf, BUF_SIZE)) > 0)
    {
        //printf("count: %d\tbuf: (%s)\n", count, buf);
        write(logtxt_fd, buf, count);
    }
    //perror(strerror(errno));
    //printf("%d\n", count);
    close(logtxt_fd);
}

int main(int argc, char** argv)
{
    if ((argc > 1) && (!strcmp("clean", argv[1])))
    {
        unlink(my_fifo);
        printf("Fifo was cleaned\n");
    }

    char src[PATH_SIZE];
    char dest[PATH_SIZE];
    char beh[10];

    printf("MAN:\n");
    printf("Start new daemon:\t \"start (src_path) (dest_path) (symlink behaviour: file/link)\"\n");
    printf("Stop existing daemon:\t \"stop (pid)\"\n");
    printf("Change existing daemon:\t \"change (pid) (src_path) (dest_path) (symlink behaviour: file/link)\"\n");
    printf("Copy log from fifo to file:\t \"log\"\n");

    create_fifo();
    while(1)
    {
        scanf ("%s", buf);
        if (!strcmp(buf, "start"))
        {
            scanf("%s %s %s", src, dest, beh);
            start(src, dest, beh);
        }
        else if (!strcmp(buf, "stop"))
        {
            pid_t pid;
            scanf("%d", &pid);
            stop(pid);
            printf("stopped successfully\n");
        }
        else if (!strcmp(buf, "change"))
        {
            pid_t pid;
            int symlink_beh;
            scanf("%d %s %s %s", &pid, src, dest, beh);
            change(pid, src, dest, beh);
        }
        else if (!strcmp(buf, "log"))
        {
            write_log();
            printf("logged in file successfully\n");
        }
    }
}