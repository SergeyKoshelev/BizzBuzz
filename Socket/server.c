#include "lib.h"

#include <arpa/inet.h>
const char server_ip[] = "127.0.0.1";
char log_str[BUFSZ];

FILE* logfile = NULL;

//start shell, create fd
void start_shell(int* shellfd)
{
    int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd < 0)
        perror("Open fd in start shell");
    
    *shellfd = fd;

    if (grantpt(fd) < 0)
        perror("grantpt");
    if (unlockpt(fd) < 0)
        perror("unlockpt");
    char* path = ptsname(fd);
    if (path == NULL)
        perror("ptsname");
    int res_fd = open(path, O_RDWR);
    if (res_fd < 0)
        perror("open res_fd in start shell");
    struct termios termios_p;
    termios_p.c_lflag = 0;
    tcsetattr(res_fd, 0, &termios_p);
        //perror("tcsetattr");
    int pid = fork();
    if (pid == 0) {
        if (dup2(res_fd, STDIN_FILENO) < 0)
            perror("dup2 res_fd stdin");
        if (dup2(res_fd, STDOUT_FILENO) < 0)
            perror("dup2 res_fd stdout");
        if (dup2(res_fd, STDERR_FILENO) < 0)
            perror("dup2 res_fd stdrerr");
        if (setsid() < 0)
            perror("setsid");
        execl("/bin/bash", "/bin/bash", NULL);
        perror("execl");
        exit(1);
    }

    char trash_buf[BUFSZ];
    read(*shellfd, trash_buf, BUFSZ);
}

//doing command from buffer
//fuction for subproccess 
int handler (char* buffer, int* shellfd)
{
      
    if (*shellfd > 0)  //if shell is activated
    {
        buffer[strlen(buffer)] = '\n';
        //printf("(%s)", buffer);
        int ret = write(*shellfd, buffer, strlen(buffer));

        if (starts_with(buffer, EXIT))  //if exit shell
        {
            *shellfd = -1;
            printf("Exit from shell\n");
        }
        else   //if command for shell (not exit)
        {    
            struct pollfd poll_info = {*shellfd, POLLIN};
            while (ret = poll(&poll_info, 1, TIMEOUT) != 0) 
            {
                clear_buf(buffer, BUFSZ);
                ret = read(*shellfd, buffer, BUFSZ);
                if (ret < 0)
                    perror("read from shellfd");
        
                printf("%s", buffer);
            }
        }
        return 1;
    }

    //if shell if not activated
    if (starts_with(buffer, PRINT))
    {
        printf("%s\n", buffer + sizeof(PRINT));
        //send_buf(sk, name, buffer+sizeof(PRINT), -1);
        return 1;
    }
    else if (starts_with(buffer, LS))
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            execlp("ls", "ls", NULL);
            perror("Error in exec in LS");
            exit(1);
        }
        waitpid(pid, NULL, 0);
        return 1;
    }
    else if (starts_with(buffer, CD))
    {
        char* path = buffer + sizeof(CD);
        int ret = chdir(path);
        if (ret < 0)
            perror("Cant change directory");
        else
            printf("New directory: (%s)\n", path);
        return 1;
    }
    else if (starts_with(buffer, SHELL))
    {
        if (*shellfd < 0)
            start_shell(shellfd);
        if (*shellfd > 0)
            printf("Shell started\n");
        else
            printf("Error with starting shell\n");
        
        return 1;
    }
    else
    {
        printf("UNKNOWN COMMAND: (%s)\n", buffer);
        return 1;
    }
    
}

int main(int argc, char** argv)
{
    assert(argc > 1);
    start_daemon();
    logfile = stdout;
    int main_pid = getpid();
    int pid = main_pid;

    logfile = open_log_file();
    fprintf(logfile, "\nStart server with pid: %d\n", main_pid);
    printf("Start server with pid: %d\n", main_pid);
    unlink(PATH);
    
    void* sl = choose_protocol(argv[1], logfile);
    int (*create_socket)();
    void (*listen_socket)(int sk, int count);
    int (*master)(int sk, struct sockaddr_in* name, 
           char* buffer, char* data, client_info* clients, 
           int* clients_count, int* pipe_from_fd, 
           int* client_sk, FILE* logfile);
    int (*slave)(int sk, struct sockaddr_in* name, 
          int pipe_from_fd, int client_sk,
          int (*handler)(char* buffer, int* shellfd),
          void (*shell_start)(int* shellfd),
          void (*send_buf)(int sk, struct sockaddr_in* name, char* data, int client_sk));
    int (*receive_buf)(int sk, struct sockaddr_in* name, char* buffer, int client_sk);
    void (*send_buf)(int sk, struct sockaddr_in* name, char* data, int client_sk);
    *(void **) (&create_socket) = dlsym(sl, "create_socket");
    *(void **) (&listen_socket) = dlsym(sl, "listen_socket");
    *(void **) (&master) = dlsym(sl, "master");
    *(void **) (&slave) = dlsym(sl, "slave");
    *(void **) (&receive_buf) = dlsym(sl, "receive_buf");
    *(void **) (&send_buf) = dlsym(sl, "send_buf");

    int sk, ret, flag = 1, clients_count = 0, new = 0, pipe_from_fd, count, client_sk;
    struct sockaddr_in name = {0};
    
    client_info* clients = (client_info*)malloc(MAX_CLIENTS_COUNT * sizeof(client_info));

    struct in_addr addr = {INADDR_ANY}; //for accepting all incoming messages, server_ip become useless
    //convert_address(server_ip, &addr); //how to save from INADDR_ANY?
    sk = create_socket();
    //close(sk);
    //exit(0);
    create_sock_name(&name, addr);
    bind_socket(sk, name);
    listen_socket(sk, 20);

    while (flag)
    {
        char data[BUFSZ] = {0};
        char buffer[BUFSZ] = {0};
        if ((logfile != stdout) && (logfile != NULL))
            fclose(logfile);

        flag = (*master)(sk, &name, buffer, data, clients, &clients_count, &pipe_from_fd, &client_sk, logfile);
        if (flag == -1) //child want to escape
        {
            pid = 0;
            break;
        } 
    }


    //code for child, read data from its pipe and do command from data
    int shellfd = -1;
    if (pid == 0)
    {
        int (*handler_p)(char* buffer, int* shellfd) = handler;
        void (*start_shell_p)(int* shellfd) = start_shell;
        (*slave)(sk, &name, pipe_from_fd, client_sk, handler_p, start_shell_p, send_buf);
    }
    else //code for main process of server, close server
    {
        fprintf(logfile, "End of server\n");
        unlink(PATH);
        free(clients);
    }
}