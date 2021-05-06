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

    // char trash_buf[BUFSZ];
    //read(*shellfd, trash_buf, BUFSZ);
    return 1;
}

//doing command from buffer
//fuction for subproccess 
int handler (char* buffer, int* shellfd)
{
    int ret;
    struct pollfd poll_info;
    int command = get_command(buffer);  
    if (*shellfd > 0)  //if shell is activated
    {
        //buffer[strlen(buffer)] = '\n';
        //printf("(%s)", buffer);
        ret = write(*shellfd, buffer, strlen(buffer));
        if (ret < 0){
            log_err("Cant write to shell");
            return -1;
        }

        switch (command) {
            case exit_com:
                *shellfd = -1;
                printf("Shell deactivated\n");
                break;
            default:
                poll_info = (struct pollfd){*shellfd, POLLIN};
                while ((ret = poll(&poll_info, 1, TIMEOUT)) > 0) {
                    clear_buf(buffer, BUFSZ);
                    ret = read(*shellfd, buffer, BUFSZ);
                    if (ret < 0)
                        log_err("read from shellfd");
        
                    printf("%s", buffer);
                }
        }
        return 1;
    }

    //if shell if not activated
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
        case exit_com:{
            printf("Exit from slave process\n");
            return 0;
        }
        default:{
            printf("UNKNOWN COMMAND: (%s)\n", buffer);
            return 1;
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 2){
        printf("Not enough arguments. Should be ./server [protocol]\n");
        return -1;
    }
    start_daemon();
    int main_pid = getpid();
    int pid = main_pid;

    int ret = log_init("server.log");
    if (ret < 0)
        printf("Can't start log in logfile\n");
    
    log_info("Start server with pid: %d", main_pid);
    printf("Start server with pid: %d\n", main_pid);
    unlink(PATH);
    
    void* sl = choose_protocol(argv[1]);
    if (sl == NULL){
        log_err("Bad shared library pointer");
        return -1;
    }
    int (*create_socket)() = dlsym(sl, "create_socket");
    int (*listen_socket)(int sk, int count) = dlsym(sl, "listen_socket");
    int (*master)(int sk, struct sockaddr_in* name,  
                  int (*handler)(char* buffer, int* shellfd)) = dlsym(sl, "master");;

    int sk, flag = 1;
    struct sockaddr_in name = {0};

    struct in_addr addr = {INADDR_ANY}; //for accepting all incoming messages, server_ip become useless
    //convert_address(server_ip, &addr); //how to save from INADDR_ANY?
    sk = create_socket();
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
    ret = listen_socket(sk, 20);
    if (ret < 0){
        log_err("Unable to listen socket");
        close(sk);
        return -1;
    }

    ret = setsockopt(sk, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
    if (ret < 0)
        log_err("setsockopt can't change mode");
    ret = setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (ret < 0)
        log_err("setsockopt can't change mode");

    flag = master(sk, &name, handler);

    if ((pid = getpid()) == main_pid)
    {
        log_info("End of server master");
        killpg(0, SIGINT);
    }
    else
        log_info("End of server slave, pid: %d", pid);
}