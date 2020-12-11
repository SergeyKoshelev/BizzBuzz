#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_flock(struct flock fl)
{
    printf("LOCK INFO:\n");
    printf("Lock type: ");
    switch (fl.l_type) {
        case F_WRLCK: 
            printf("F_WRLCK\n");
            break;
        case F_RDLCK:
            printf("F_RDLCK\n");
            break;
        default:
            printf("UNKNOWN\n");
    }

    printf("Whence: ");
    switch (fl.l_whence) {
        case SEEK_CUR: 
            printf("SEEK_CUR\n");
            break;
        case SEEK_END:
            printf("SEEK_END\n");
            break;
        case SEEK_SET:
            printf("SEEK_SET\n");
            break;
        default:
            printf("UNKNOWN\n");
    }

    printf("Length: %ld\n", fl.l_len);
    printf("Pid: %d\n", fl.l_pid);
    printf("Start offset: %ld\n\n", fl.l_start);
}

int main(int argc, char** argv)
{
    assert(argc > 1);
    struct flock fl = {0};
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    int fd = open(argv[1], O_RDWR);
    assert(fd > 0);   
    
    int ret = fcntl(fd, F_GETLK, &fl);
    if (ret < 0)
    {
        printf("error\n");
        close(fd);
    }
    print_flock(fl);
    
    close(fd);
}