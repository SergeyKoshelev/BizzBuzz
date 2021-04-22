#include "lib.h"

//UDP function
//create AF_INET, SOCK_DGRAM socket
int create_socket()
{
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        perror("Unable to create socket");
        exit(1);
    } 

    return sk;
}

//UDP function
//only size of BUFSZ
//can be used only to connect for "findall" and for sending command from client
//need only sk(write in it), name, data
void send_buf(int sk, struct sockaddr_in* name, char* data, int client_sk)
{
    int ret = sendto(sk, data, BUFSZ, 0, (struct sockaddr*)name, sizeof(*name)); //send findall command
    if (ret < 0)
    {
        perror("Unable to write");
        exit(1);
    }
}

//UDP function
//only size of BUFSZ
//can be used only for receive command in server and for receive "findall" request in client
//need only sk, name, buffer as arguments
int receive_buf(int sk, struct sockaddr_in* name, char* buffer, int client_sk)
{
    socklen_t fromlen = sizeof(struct sockaddr_in);
    int size = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)name, &fromlen);
    buffer[BUFSZ] = '\0';

    if (size < 0 || size > BUFSZ)
        printf("Unexpected read error or overflow %d\n", size);

    return strlen(buffer);
}

//useless function for UDP 
//(using only for TCP)
void connect_socket (int sk, struct sockaddr_in name){};

//useless function for UDP 
//(using only for TCP)
void listen_socket (int sk, int count){};

//useless function for UDP 
//(using only for TCP)
int accept_socket (int sk){};

//UDP function
//get array of clients_info and cliend_id
//return [position] in array, set new 1 if new and 0 if old
int check_clients_info(client_info* clients, int client_id, int* clients_count, int* new_client, FILE* logfile)
{
    int i = 0;
    for (i = 0; i < *clients_count; i++)
        if (clients[i].client_id == client_id) //if such client exists
        {
            *new_client = 0;
            return i;
        }

    if (i < MAX_CLIENTS_COUNT)  //if new client and not overload in clients array
    {
        clients[i].client_id = client_id;
        int ret = pipe(clients[i].pipes_from_main);
        if (ret < 0)
            fprintf(logfile, "creating pipes from main to subprocess for new client: %s\n", strerror(errno));
        clients[i].shell = 0;
        *clients_count += 1;
        *new_client = 1;
        return i;
    }
}

//disconnect server, closes pipes and replace cell of client 
int client_disconnect(client_info* clients, int position, int* clients_count, FILE* logfile)
{
    //clear_buf(log_str, BUFSZ);
    //sprintf(log_str, "id: %d\tDisconnected\n", clients[position].client_id);
    //int count = write(fifo_fd, log_str, strlen(log_str));
    //assert(count > 0);
    fprintf(logfile, "id: %d\tDisconnected\n", clients[position].client_id);

    int ret = kill(clients[position].pid, SIGKILL);
    if (ret < 0)
        fprintf(logfile, "kill in client disconnect: %s\n", strerror(errno));
    clients[position].client_id = 0;
    ret = close(clients[position].pipes_from_main[0]); //close input in pipe
    if (ret < 0)
        fprintf(logfile, "close pipe from main, 0 in client disconnecting: %s\n", strerror(errno));
	ret = close(clients[position].pipes_from_main[1]); //close output from pipe
    if (ret < 0)
        fprintf(logfile, "close pipe from main, 1 in client disconnecting: %s\n", strerror(errno));
    *clients_count -= 1;
    //printf("count: %d\t position: %d\n", *clients_count, position);
    if (*clients_count != position) //not last elem of array, move last elem to empty cell
    {
        clients[position].client_id = clients[*clients_count].client_id;
        clients[position].pipes_from_main[0] = clients[*clients_count].pipes_from_main[0];
        clients[position].pipes_from_main[1]= clients[*clients_count].pipes_from_main[1];
        clients[position].pid = clients[*clients_count].pid;

        clients[*clients_count].client_id = 0;
    }
}

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

//master for UDP
int master(int sk, struct sockaddr_in* name, 
           char* buffer, char* data, client_info* clients, 
           int* clients_count, int* pipe_from_fd, 
           int* client_sk, FILE* logfile)
{
    receive_buf(sk, name, buffer, -1);
    logfile = open_log_file();

    int flag = 0;
    int client_id = separate_buffer(buffer, data);
    int position = check_clients_info(clients, client_id, clients_count, &flag, logfile);
    
        if (starts_with(data, FINDALL)) //if command findall
        {
            fprintf(logfile, "FINDALL\n");
            send_buf(sk, name, "server", -1);
        }
        else if ((flag == 1) && (starts_with(data, EXIT)))  //if new client and command exit
            return 1; //ignore it
        else if (starts_with(data, EXIT) && (clients[position].shell == 0))  //if command exit and server's part not in shell
            client_disconnect(clients, position, clients_count, logfile);
        else 
        {
            int ret = write(clients[position].pipes_from_main[1], data, strlen(data));
            if (ret < 0)
                fprintf(logfile, "write in pipe to subprocess: %s\n", strerror(errno));

            if (flag == 1) //if new client
            {
                *pipe_from_fd = clients[position].pipes_from_main[0];
                int pid = fork();
                if (pid == 0) //child
                {
                    free(clients);
                    return -1;
                }
                
                fprintf(logfile, "id: %d\tConnected\n", clients[position].client_id);   //stdout can't be replaced!?!?!?!?!
                clients[position].pid = pid;
            }

            fprintf(logfile, "Id: %d\tCommand: %s\n", clients[position].client_id, data);

            if (starts_with(data, EXIT))  //if command exit in shell
            {
                fprintf(logfile, "id: %d\tDeactivated shell\n", clients[position].client_id);
                clients[position].shell = 0;
            }
            else if (starts_with(data, SHELL)) //if command shell
            {
                fprintf(logfile, "id: %d\tStarting shell\n", clients[position].client_id);
                clients[position].shell = 1;
            }

        }
        return 1;
}

//slave for UDP
int slave(int sk, struct sockaddr_in* name, 
          int pipe_from_fd, int client_sk, 
          int (*handler)(char* buffer, int* shellfd),
          void (*shell_start)(int* shellfd),
          void (*send_buf)(int sk, struct sockaddr_in* name, char* data, int client_sk))
{
    int buf_pipe[2];
    pipe(buf_pipe);

    int shellfd = -1;

    int ret = dup2(buf_pipe[1], STDOUT_FILENO);
    if (ret < 0)
        perror("dup2 in starting subprocess, stdout");
    ret = dup2(buf_pipe[1], STDERR_FILENO);
    if (ret < 0)
        perror("dup2 in starting subprocess, stderr");

    int flag = 1;
    while (flag)
    {
        char data[BUFSZ] = {0};
        int ret = read(pipe_from_fd, data, BUFSZ);  //read command from main server
        if (ret < 0)
            perror("read in subprocess from main pipe");
        flag = handler(data, &shellfd); //do command
        send_data(sk, name, data, buf_pipe[0], -1, send_buf);
    }
}