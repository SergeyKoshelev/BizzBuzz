#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

const char str[] = "Hello, locked world!\n";

int main(int argc, char** argv)
{
    assert(argc > 1);
    struct flock f1 = {0};
    struct flock f2 = {0};

    f1.l_type = F_RDLCK;
    f1.l_whence = SEEK_SET;
    f1.l_len = 1;
    f1.l_pid = 0;

    f2.l_type = F_WRLCK;
    f2.l_whence = SEEK_END;
    f2.l_len = -1;
    f2.l_pid = 0;
    

    int fd = open(argv[1], O_RDWR);
    assert(fd > 0);
    write(fd, str, strlen(str));

    int ret;
    ret = fcntl(fd, F_SETLK, &f1);
    if (ret < 0)
    {
        printf("can't set lock\n");
        close(fd);
        exit(0);
    }
    printf("set RDLCK\n");

    ret = fcntl(fd, F_SETLK, &f2);
    if (ret < 0)
    {
        printf("can't set lock\n");
        close(fd);
        exit(0);
    }
    printf("set WRLCK\n");

    sleep(1000);
    close(fd);
}