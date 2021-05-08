#include "lib.h"
#include "log.h"

#include <arpa/inet.h>
const char server_ip[] = "127.0.0.1";

//start shell, create fd
int start_shell(int* shellfd)
{
    int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd < 0){
        log_err("Open fd in start shell");
        return -1;
    }
    *shellfd = fd;
    if (grantpt(fd) < 0){
        log_err("grantpt");
        return -1;
    }
    if (unlockpt(fd) < 0){
        log_err("unlockpt");
        return -1;
    }
    char* path = ptsname(fd);
    if (path == NULL){
        log_err("ptsname");
        return -1;
    }
    int res_fd = open(path, O_RDWR);
    if (res_fd < 0){
        log_err("open res_fd in start shell");
        return -1;
    }
    struct termios termios_p;
    termios_p.c_lflag = 0;
    tcsetattr(res_fd, 0, &termios_p);
        //perror("tcsetattr");
    int pid = fork();
    if (pid == 0) {
        log_info("bash pid: %d", getpid());
        if (dup2(res_fd, STDIN_FILENO) < 0){
            log_err("dup2 res_fd stdin");
            return -1;
        }
        if (dup2(res_fd, STDOUT_FILENO) < 0){
            log_err("dup2 res_fd stdout");
            return -1;
        }
        if (dup2(res_fd, STDERR_FILENO) < 0){
            log_err("dup2 res_fd stdrerr");
            return -1;
        }
        if (setsid() < 0){
            log_err("setsid");
            return -1;
        }
        execl("/bin/bash", "/bin/bash", NULL);
        log_err("execl");
        exit(1);
    }

    return 1;
}

//handler if bash is executing
int bash_handler (char* buffer, int* shellfd, int command)
{
    int ret;
    struct pollfd poll_info;
    if (command == quit)
    {
        ret = write(*shellfd, "exit\n", strlen("exit\n"));  //close all bashes
        if (ret < 0){
            log_err("Cant write to shell");
            return -1;
        }
        printf("Quit from slave process\n");
        return 0;
    }
    else
    {
        log_info("(%s)", buffer);
        ret = write(*shellfd, buffer, strlen(buffer));
        if (ret < 0){
            log_err("Cant write to shell");
            return -1;
        }
        poll_info = (struct pollfd){*shellfd, POLLIN};
        while ((ret = poll(&poll_info, 1, TIMEOUT)) > 0) {
            clear_buf(buffer, BUFSZ);
            ret = read(*shellfd, buffer, BUFSZ);
            if (ret < 0)
                log_err("read from shellfd");
                printf("%s", buffer);
        }
        return 1;
    }
}

//handler if bash is not executing
int usual_handler(char* buffer, int* shellfd, int command)
{
    int ret;
    switch (command){
        case print:{
            printf("%s\n", buffer + sizeof(PRINT));
            return 1;
        }
        case ls:{
            pid_t pid = fork();
            if (pid == 0)
            {
                execlp("ls", "ls", NULL);
                log_err("Error in exec in LS");
                exit(1);
            }
            waitpid(pid, NULL, 0);
            return 1;
        }
        case cd:{
            char* path = buffer + sizeof(CD);
            ret = chdir(path);
            if (ret < 0)
                perror("Cant change directory");
            else
                printf("New directory: (%s)\n", path);
            return 1;
        }
        case shell:{
            ret = start_shell(shellfd);
            if ((*shellfd > 0)&&(ret > 0))
                printf("Shell started\n");
            else{
                printf("Error with starting shell\n");
                *shellfd = -1;
            }
            return 1;
        }
        case quit:{
            printf("Quit from slave process\n");
            return 0;
        }
        default:{
            printf("UNKNOWN COMMAND: (%s)\n", buffer);
            return 1;
        }
    }
}

//doing command from buffer
//fuction for subproccess 
int handler (char* buffer, int* shellfd)
{
    int command = get_command(buffer);  
    if (*shellfd > 0)
        return bash_handler(buffer, shellfd, command);
    else
        return usual_handler(buffer, shellfd, command); 
  }

int main(int argc, char** argv){
    if (argc < 2){
        printf("Not enough arguments. Should be ./server [protocol]\n");
        return -1;
    }
    start_daemon();
    int main_pid = getpid();
    int pid = main_pid;
    log_init("server.log");
    log_info("Start server with pid: %d", main_pid);
    printf("Start server with pid: %d\n", main_pid);
    functions functions = get_functions(argv[1]);
    if (functions.success == error){
        log_err("error with getting shared library functions");
        return -1;             
    }

    int sk, flag = 1, ret;
    struct sockaddr_in name = {0};
    struct in_addr addr = {INADDR_ANY}; //for accepting all incoming messages, server_ip become useless
    //convert_address(server_ip, &addr); //how to save from INADDR_ANY?
    sk = functions.create_socket();
    if (sk < 0){
        log_err("Can't create socket");
        return -1;
    }
    create_sock_name(&name, addr);
    ret = bind_socket(sk, name);
    if (ret < 0){
        log_err("Can't bind socket");
        close(sk);
        return -1;
    }
    ret = functions.listen_socket(sk, 20);
    if (ret < 0){
        log_err("Unable to listen socket");
        close(sk);
        return -1;
    }
    ret = setsockopt(sk, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
    if (ret < 0)
        log_err("setsockopt can't change mode reuseport");
    ret = setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (ret < 0)
        log_err("setsockopt can't change mode reuseaddr");

    flag = functions.master(sk, &name, handler);
    if ((pid = getpid()) == main_pid){
        log_info("End of server master");
        killpg(0, SIGINT);
    }
    else
        log_info("End of server slave, pid: %d", pid);
}