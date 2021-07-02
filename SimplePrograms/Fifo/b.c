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

    int fd_fifo = open(my_fifo, O_RDONLY | O_NONBLOCK);
    if (fd_fifo == -1) 
    {
        printf("Can't open fifo %s for logging\n", my_fifo);
        exit(1);
    }
    int empty;
    scanf("%d", &empty);
    printf("fd: %d\n", fd_fifo);
    char buf[1024];
    int count;
    while ((count = read(fd_fifo, buf, 1024)) > 0)
    {
        buf[count] = '\0'; 
        printf("count: %d (%s)", count, buf);
        //write(logtxt_fd, buf, BUF_SIZE);
    }
}