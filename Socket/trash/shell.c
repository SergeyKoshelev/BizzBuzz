#include "UDP.h"

void start_shell(int* shellfd)
{
    int ret, resfd, pid;
    int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd < 0)
        perror("open fd");
    
    *shellfd = fd;

    if (grantpt(fd) < 0)
        perror("grantpt");
    if (unlockpt(fd) < 0)
        perror("unlockpt");
    char* path = ptsname(fd);
    if (path == NULL)
        perror("ptsname");
    resfd = open(path, O_RDWR);
    if (resfd < 0)
        perror("open resfd");
    struct termios termios_p;
    termios_p.c_lflag = 0;
    tcsetattr(resfd, 0, &termios_p);
        //perror("tcsetattr");
    pid = fork();
    if (pid == 0) {
        if (dup2(resfd, STDIN_FILENO) < 0)
            perror("dup2 resfd stdin");
        if (dup2(resfd, STDOUT_FILENO) < 0)
            perror("dup2 resfd stdout");
        if (dup2(resfd, STDERR_FILENO) < 0)
            perror("dup2 resfd stdrerr");
        if (setsid() < 0)
            perror("setsid");
        execl("/bin/bash", "/bin/bash", NULL);
        perror("execl");
        exit(1);
    }
}

int main()
{
    int fd = -1;
    char command[] = "ls\n";
    char buffer[BUFSZ];
    start_shell(&fd);
    int ret = write(fd, command, strlen(command));
    usleep(500);
    //ret = read(fd, buffer, BUFSZ);
    clear_buf(buffer, BUFSZ);  
    ret = read(fd, buffer, BUFSZ);  
    printf("(%s)", buffer);
}
