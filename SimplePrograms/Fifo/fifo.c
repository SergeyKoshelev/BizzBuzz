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

int main()
{
    char my_fifo[] = "fifo";
    unlink(my_fifo);
    int check_mk = mkfifo(my_fifo, S_IRUSR | S_IWUSR);
    if (check_mk == -1)
    {
        printf("Can't create fifo %s for logging\n", my_fifo);
        exit(1);
    }

    int fd_fifo = open(my_fifo, O_RDWR);
    if (fd_fifo == -1) 
    {
        printf("Can't open fifo %s for logging\n", my_fifo);
        exit(1);
    }
    printf("fd: %d\n", fd_fifo);
    write(fd_fifo, "str\t", strlen("str\t"));
    write(fd_fifo, "hello\n", strlen("hello\n"));
    char buf[1024];
    while (read(fd_fifo, buf, 1024) > 0)
    {
        printf("%s", buf);
        //write(logtxt_fd, buf, BUF_SIZE);
    }
    close(fd_fifo);
}