#include "lib.h"

#include <arpa/inet.h>
const char server_ip[] = "127.0.0.1";
char log_str[BUFSZ];

FILE* logfile = NULL;
int fifo_fd = -1;

//separate received buffer on id and data
//returns id, data in variable data
int separate_buffer(char * buffer, char* data) 
{
    char* space = strchr(buffer, ' ');
    *space = '\0';
    int client_id = atoi(buffer);
    strcpy(data, space + 1);
    //printf("(%d) (%s)\n", client_id, data);
    return client_id;
}

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

//delegate work to subprocess using pipes (just write request in pipe)
int delegate(client_info* clients, int position, char* data, int fifo_fd)
{
    int ret = write(clients[position].pipes_from_main[1], data, strlen(data));
    if (ret < 0)
        fprintf(logfile, "write in pipe to subprocess: %s\n", strerror(errno));
}

int main()
{
    start_daemon();
    logfile = stdout;
    int main_pid = getpid();
    int pid = main_pid;

    logfile = open_log_file();
    fprintf(logfile, "\nStart server with pid: %d\n", main_pid);
    printf("Start server with pid: %d\n", main_pid);
    unlink(PATH);
    

    int sk, ret, flag = 1, clients_count = 0, new = 0, pipe_to_fd, pipe_from_fd, position, count;
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
    int client_sk = accept_socket(sk);

    while (1)
    {
        char data[BUFSZ] = {0};
        char buffer[BUFSZ] = {0};
        if ((logfile != stdout) && (logfile != NULL))
            fclose(logfile);

        receive_buf(sk, &name, buffer, client_sk);
        logfile = open_log_file();

        int client_id = separate_buffer(buffer, data);
        //printf("id: %d\tdata:(%s)\n", client_id, data);
        position = check_clients_info(clients, client_id, &clients_count, &flag);
    
        if (starts_with(data, FINDALL)) //if command findall
        {
            fprintf(logfile, "FINDALL\n");
            send_buf(sk, &name, "server", client_sk);
        }
        else if ((flag == 1) && (starts_with(data, EXIT)))  //if new client and command exit
            continue; //ignore it
        else if (starts_with(data, EXIT) && (clients[position].shell == 0))  //if command exit and server's part not in shell
            client_disconnect(clients, position, &clients_count);
        else 
        {
            delegate(clients, position, data, fifo_fd);

            if (flag == 1) //if new client
            {
                pipe_from_fd = clients[position].pipes_from_main[0];
                pipe_to_fd = clients[position].pipes_to_main[1];
                pid = fork();
                if (pid == 0) //child
                {
                    free(clients);
                    break;
                }
                
                //clear_buf(log_str, BUFSZ);
                //sprintf(log_str, "id: %d\tConnected\n", clients[position].client_id);
                //count = write(fifo_fd, log_str, strlen(log_str));
                //assert(count > 0);
                fprintf(logfile, "id: %d\tConnected\n", clients[position].client_id);   //stdout can't be replaced!?!?!?!?!
                clients[position].pid = pid;
            }

            //clear_buf(log_str, BUFSZ);
            //sprintf(log_str, "Id: %d\tCommand: %s\n", clients[position].client_id, data);
            //int count = write(fifo_fd, log_str, strlen(log_str));
            //assert(count > 0);
            fprintf(logfile, "Id: %d\tCommand: %s\n", clients[position].client_id, data);

            if (starts_with(data, EXIT))  //if command exit in shell
            {
                //clear_buf(log_str, BUFSZ);
                //sprintf(log_str, "id: %d\tDeactivated shell\n", clients[position].client_id);
                //count = write(fifo_fd, log_str, strlen(log_str));
                //assert(count > 0);
                fprintf(logfile, "id: %d\tDeactivated shell\n", clients[position].client_id);

                clients[position].shell = 0;
            }
            else if (starts_with(data, SHELL)) //if command shell
            {
                //clear_buf(log_str, BUFSZ);
                //sprintf(log_str, "id: %d\tStarting shell\n", clients[position].client_id);
                //count = write(fifo_fd, log_str, strlen(log_str));
                //assert(count > 0);
                fprintf(logfile, "id: %d\tStarting shell\n", clients[position].client_id);
                clients[position].shell = 1;
            }

            send_data(sk, &name, data, clients, position, client_sk);
        } 
      
    }


    //code for child, read data from its pipe and do command from data
    int shellfd = -1;
    if (pid == 0)
    {
        int ret = dup2(pipe_to_fd, STDOUT_FILENO);
        if (ret < 0)
            perror("dup2 in starting subprocess, stdout");
        ret = dup2(pipe_to_fd, STDERR_FILENO);
        if (ret < 0)
            perror("dup2 in starting subprocess, stderr");
        flag = 1;
        while (flag)
        {
            char data[BUFSZ] = {0};
            ret = read(pipe_from_fd, data, BUFSZ);  //read command from main server
            if (ret < 0)
                perror("read in subprocess from main pipe");
            flag = handler(data, &shellfd); //do command
        }
    }
    else //code for main process of server, close server
    {
        fprintf(logfile, "End of server\n");
        unlink(PATH);
        free(clients);
    }
}