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

//doing command from data
int handler (char* buffer)
{
    //printf("buffer: (%s)\n", buffer);
    if (!strncmp(buffer, PRINT, sizeof(PRINT) - 1))
    {
        printf("%s\n", buffer + sizeof(PRINT));
        return 1;
    }
    else if (!strncmp(buffer, LS, sizeof(LS) - 1))
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
    else if (!strncmp(buffer, CD, sizeof(CD) - 1))
    {
        char* path = buffer + sizeof(CD);
        printf("new directory: (%s)\n", path);
        int ret = chdir(path);
        if (ret < 0)
        {
            perror("Cant change directory\n");
            exit(1);
        }
        return 1;
    }
    else
    {
        printf("UNKNOWN COMMAND\n");
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
        printf("connecting with new client\n");
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
    printf("disconnect client, id: %d\n", clients[position].client_id);
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

int main()
{
    unlink(PATH);

    int sk, ret, flag = 1, clients_count = 0, new = 0, pipe_to_fd, pipe_from_fd, position, count;
    struct sockaddr_in name = {0};
    socklen_t fromlen = sizeof(name);
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
        ret = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, &fromlen);

        if (ret < 0 || ret > BUFSZ)
        {
            printf("Unexpected read error or overflow %d\n", ret);
            exit(1);
        }

        int client_id = separate_buffer(buffer, data);
        //printf("id: %d\tdata:(%s)\n", client_id, data);
        position = check_info(clients, client_id, &clients_count, &flag);
    
        if (!strncmp(data, FINDALL, sizeof(FINDALL) - 1)) //if command findall
        {
            printf("get findall\n");
            
            ret = sendto(sk, "server", BUFSZ, 0, (struct sockaddr*)&name, sizeof(name)); //send response on findall
            if (ret < 0)
            {
                perror("Unable to write");
                exit(1);
            }
        }
        else if ((flag == 1) && (strncmp(data, EXIT, sizeof(EXIT) - 1)))  //if new client and not command close
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
                write(clients[position].pipes_from_main[1], data, strlen(data));
                count = read(clients[position].pipes_to_main[0], data, BUFSZ);
                data[count] = '\0';
                ret = sendto(sk, data, BUFSZ, 0, (struct sockaddr*)&name, sizeof(name)); //send findall command
                if (ret < 0)
                {
                    perror("Unable to write");
                    exit(1);
                }
            } 
        }
        else if ((flag == 1) && (!strncmp(data, EXIT, sizeof(EXIT) - 1)))  //if new client and command close
            continue; //ignore it
        else if (!strncmp(data, EXIT, sizeof(EXIT) - 1))
            client_disconnect(clients, position, &clients_count);
        else
        {
            write(clients[position].pipes_from_main[1], data, strlen(data));
            count = read(clients[position].pipes_to_main[0], data, BUFSZ);
            data[count] = '\0';
            ret = sendto(sk, data, BUFSZ, 0, (struct sockaddr*)&name, sizeof(name)); //send findall command
            if (ret < 0)
            {
                perror("Unable to write");
                exit(1);
            } 
        }     
    }


    //code for child, read data from its pipe and do command from data
    if (pid == 0)
    {
        dup2(pipe_to_fd, STDOUT_FILENO);
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