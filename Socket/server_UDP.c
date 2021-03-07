#include "UDP.h"

#include <arpa/inet.h>
const char server_ip[] = "127.0.0.1";

struct client_info {
    int pid;
    int client_id;
    int pipes_from_main[2];
    int pipes_to_main[2];
}; typedef struct client_info client_info;

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

//doing command from buffer
//fuction for subproccess 
int handler (char* buffer)
{
    //printf("buffer: (%s)\n", buffer);
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
        if (clients[i].client_id == client_id)
        {
            *new = 0;
            return i;
        }

    if (i < MAX_CLIENTS_COUNT)
    {
        printf("Connected with new client, id: %d\n", client_id);
        clients[i].client_id = client_id;
        pipe(clients[i].pipes_from_main);
        pipe(clients[i].pipes_to_main);
        *clients_count += 1;
        *new = 1;
        return i;
    }
}

//disconnect server, closes pipes and replace cell of client 
int client_disconnect(client_info* clients, int position, int* clients_count)
{
    printf("Client disconnected, id: %d\n", clients[position].client_id);
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

//delegate work to subprocess using pipes
int delegate(client_info* clients, int position, char* data)
{
    printf("Id: %d\tCommand: %s\n", clients[position].client_id, data);
    write(clients[position].pipes_from_main[1], data, strlen(data));
    int count = read(clients[position].pipes_to_main[0], data, BUFSZ);
    data[count] = '\0';
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
        receive_data(sk, &name, buffer);

        int client_id = separate_buffer(buffer, data);
        //printf("id: %d\tdata:(%s)\n", client_id, data);
        position = check_info(clients, client_id, &clients_count, &flag);
    
        if (starts_with(data, FINDALL)) //if command findall
        {
            printf("Command: findall\n");
            send_data(sk, &name, "server");
        }
        else if ((flag == 1) && (!starts_with(data, EXIT)))  //if new client and not command close
        {
            pipe_from_fd = clients[position].pipes_from_main[0];
            pipe_to_fd = clients[position].pipes_to_main[1];
            pid = fork();
            if (pid == 0) //child
            {
                free(clients);
                break;
            }
            else //parent
            {
                clients[position].pid = pid;
                delegate(clients, position, data);
                send_data(sk, &name, data);
            } 
        }
        else if ((flag == 1) && (starts_with(data, EXIT)))  //if new client and command close
            continue; //ignore it
        else if (starts_with(data, EXIT))
            client_disconnect(clients, position, &clients_count);
        else
        {
            delegate(clients, position, data);
            send_data(sk, &name, data);
        }     
    }


    //code for child, read data from its pipe and do command from data
    if (pid == 0)
    {
        dup2(pipe_to_fd, STDOUT_FILENO);
        dup2(pipe_to_fd, STDERR_FILENO);
        flag = 1;
        while (flag)
        {
            char data[BUFSZ] = {0};
            read(pipe_from_fd, data, BUFSZ);
            flag = handler(data);
        }
    }
    else //code for main process of server, close server
    {
        printf("End of server\n");
        unlink(PATH);
        free(clients);
    }
}