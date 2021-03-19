#include "UDP.h"

#include <arpa/inet.h>
const char server_ip[] = "127.0.0.1";

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
                    perror("read from pipe");
        
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
            int res = execlp("ls", "ls", NULL);
            printf("Error in exec in LS\n");
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
            printf("new directory: (%s)\n", path);
        return 1;
    }
    else if (starts_with(buffer, SHELL))
    {
        if (*shellfd < 0)
            start_shell(shellfd);

        printf("Shell started\n");
        return 1;
    }
    else
    {
        printf("UNKNOWN COMMAND: (%s)\n", buffer);
        return 1;
    }
    
}

//get array of clients_info and cliend_id
//return [position] in array, set new 1 if new and 0 if old
int check_info(client_info* clients, int client_id, int* clients_count, int* new)
{
    int i = 0;
    for (i = 0; i < *clients_count; i++)
        if (clients[i].client_id == client_id) //if such client exists
        {
            *new = 0;
            return i;
        }

    if (i < MAX_CLIENTS_COUNT)  //if new client and not overload in clients array
    {
        printf("id: %d\tConnected\n", client_id);
        clients[i].client_id = client_id;
        pipe(clients[i].pipes_from_main);
        pipe(clients[i].pipes_to_main);
        clients[i].shell = 0;
        *clients_count += 1;
        *new = 1;
        return i;
    }
}

//disconnect server, closes pipes and replace cell of client 
int client_disconnect(client_info* clients, int position, int* clients_count)
{
    printf("id: %d\tDisconnected\n", clients[position].client_id);
    kill(clients[position].pid, SIGKILL);
    clients[position].client_id = 0;
    close(clients[position].pipes_from_main[0]); //close input in pipe
	close(clients[position].pipes_from_main[1]); //close output from pipe
    close(clients[position].pipes_to_main[0]); //close input in pipe
	close(clients[position].pipes_to_main[1]); //close output from pipe
    *clients_count -= 1;
    //printf("count: %d\t position: %d\n", *clients_count, position);
    if (*clients_count != position) //not last elem of array, move last elem to empty cell
    {
        clients[position].client_id = clients[*clients_count].client_id;
        clients[position].pipes_from_main[0] = clients[*clients_count].pipes_from_main[0];
        clients[position].pipes_from_main[1]= clients[*clients_count].pipes_from_main[1];
        clients[position].pipes_to_main[0] = clients[*clients_count].pipes_to_main[0];
        clients[position].pipes_to_main[1]= clients[*clients_count].pipes_to_main[1];
        clients[position].pid = clients[*clients_count].pid;

        clients[*clients_count].client_id = 0;
    }
}

//delegate work to subprocess using pipes (just write request in pipe)
int delegate(client_info* clients, int position, char* data)
{
    printf("Id: %d\tCommand: %s\n", clients[position].client_id, data);
    write(clients[position].pipes_from_main[1], data, strlen(data));
    //usleep(100);
}

int main()
{
    unlink(PATH);

    int sk, ret, flag = 1, clients_count = 0, new = 0, pipe_to_fd, pipe_from_fd, position, count;
    struct sockaddr_in name = {0};
    int main_pid = getpid();
    int pid = main_pid;
    client_info* clients = (client_info*)malloc(MAX_CLIENTS_COUNT * sizeof(client_info));

    struct in_addr addr = {INADDR_ANY}; //for accepting all incoming messages, server_ip become useless
    sk = create_socket();
    create_sock_name(&name, addr);
    bind_socket(sk, name);

    while (1)
    {
        char data[BUFSZ] = {0};
        char buffer[BUFSZ] = {0};
        receive_buf(sk, &name, buffer);

        int client_id = separate_buffer(buffer, data);
        //printf("id: %d\tdata:(%s)\n", client_id, data);
        position = check_info(clients, client_id, &clients_count, &flag);
    
        if (starts_with(data, FINDALL)) //if command findall
        {
            printf("FINDALL\n");
            send_buf(sk, &name, "server");
        }
        else if ((flag == 1) && (starts_with(data, EXIT)))  //if new client and command exit
            continue; //ignore it
        else if (starts_with(data, EXIT) && (clients[position].shell == 0))  //if command exit and server's part not in shell
            client_disconnect(clients, position, &clients_count);
        else 
        {
            delegate(clients, position, data);

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
                clients[position].pid = pid;
            }

            if (starts_with(data, EXIT))  //if command exit in shell
            {
                printf("id: %d\tDeactivated shell\n", clients[position].client_id);

                clients[position].shell = 0;
            }
            else if (starts_with(data, SHELL)) //if command shell
            {
                printf("id: %d\tStarting shell\n", clients[position].client_id);
                clients[position].shell = 1;
            }

            send_data(sk, &name, data, clients, position);
        } 
      
    }


    //code for child, read data from its pipe and do command from data
    int shellfd = -1;
    if (pid == 0)
    {
        dup2(pipe_to_fd, STDOUT_FILENO);
        dup2(pipe_to_fd, STDERR_FILENO);
        flag = 1;
        while (flag)
        {
            char data[BUFSZ] = {0};
            read(pipe_from_fd, data, BUFSZ);  //read command from main server
            flag = handler(data, &shellfd); //do command
        }
    }
    else //code for main process of server, close server
    {
        printf("End of server\n");
        unlink(PATH);
        free(clients);
    }
}