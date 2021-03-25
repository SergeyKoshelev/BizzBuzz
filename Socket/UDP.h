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

//useless function for UDP (using only for TCP)
void connect_socket (int sk, struct sockaddr_in name){};

//useless function for UDP (using only for TCP)
void listen_socket (int sk, int count){};

//useless function for UDP (using only for TCP)
int accept_socket (int sk){};

//UDP function
//get array of clients_info and cliend_id
//return [position] in array, set new 1 if new and 0 if old
int check_clients_info(client_info* clients, int client_id, int* clients_count, int* new)
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
        clients[i].client_id = client_id;
        int ret = pipe(clients[i].pipes_from_main);
        if (ret < 0)
            fprintf(logfile, "creating pipes from main to subprocess for new client: %s\n", strerror(errno));
        ret = pipe(clients[i].pipes_to_main);
        if (ret < 0)
            fprintf(logfile, "creating pipes to main from subprocess for new client: %s\n", strerror(errno));
        clients[i].shell = 0;
        *clients_count += 1;
        *new = 1;
        return i;
    }
}

//disconnect server, closes pipes and replace cell of client 
int client_disconnect(client_info* clients, int position, int* clients_count)
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
    ret = close(clients[position].pipes_to_main[0]); //close input in pipe
    if (ret < 0)
        fprintf(logfile, "close pipe to main, 0 in client disconnecting: %s\n", strerror(errno));
	ret = close(clients[position].pipes_to_main[1]); //close output from pipe
    if (ret < 0)
        fprintf(logfile, "close pipe to main, 1 in client disconnecting: %s\n", strerror(errno));
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