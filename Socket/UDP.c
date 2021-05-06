#include "lib.h"
#include "log.h"

int sigchld_received = 0;

//UDP function
//create AF_INET, SOCK_DGRAM socket
int create_socket()
{
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    return sk;
}

//UDP function
//only size of BUFSZ
//can be used only to connect for "findall" and for sending command from client
//need only sk(write in it), name, data
int send_buf(int sk, struct sockaddr_in* name, char* data)
{
    encode(data);
    int ret = sendto(sk, data, BUFSZ, 0, (struct sockaddr*)name, sizeof(*name));
    if (ret < 0){
        log_err("Unable to write in %d: %s", sk, strerror(errno));
        return -1;
    }

    return ret;
}

//UDP function
//only size of BUFSZ
//can be used only for receive command in server and for receive "findall" request in client
//need only sk, name, buffer as arguments
int receive_buf(int sk, struct sockaddr_in* name, char* buffer)
{
    socklen_t fromlen = sizeof(struct sockaddr_in);
    clear_buf(buffer, BUFSZ);
    int size = recvfrom(sk, buffer, BUFSZ - 1, 0, (struct sockaddr*)name, &fromlen);

    if (size < 0 || size > (BUFSZ - 1))
    {
        log_err("Unexpected read error or overflow: %s: %d", strerror(errno), sk);
        return -1;
    }
    
    decode(buffer);
    return strlen(buffer); //because size is usually BUFSZ - 1
}

//useless function for UDP 
//(using only for TCP)
int connect_socket (int sk, struct sockaddr_in name){ return 0; };

//useless function for UDP 
//(using only for TCP)
int listen_socket (int sk, int count){ return 0; };

//try to find client in array of clients
//return position of found; else -1
int find_client(client_info* clients, int client_id, int clients_count)
{
    for (int i = 0; i < clients_count; i++)
        if (clients[i].client_id == client_id)
            return i;
    return -1;
}

//add new client in array
int add_client(client_info* clients, int client_id, int clients_count)
{
    if (clients_count >= MAX_CLIENTS_COUNT)
        return -1;

    clients[clients_count].client_id = client_id;
    int ret = pipe(clients[clients_count].pipes_from_main);
    if (ret < 0){
        log_err("creating pipes from main to subprocess for new client: %s", strerror(errno));
        return -1;
    }
    clients[clients_count].shell = 0;
    return clients_count;
}

//disconnect server, closes pipes and replace cell of client 
int client_disconnect(client_info* clients, int position, int* clients_count)
{
    while (!sigchld_received);
    sigchld_received = 0;
    clients[position].client_id = 0;
    int ret = close(clients[position].pipes_from_main[0]); //close input in pipe
    if (ret < 0)
        log_err("close pipe from main, 0 in client disconnecting: %s", strerror(errno));
	ret = close(clients[position].pipes_from_main[1]); //close output from pipe
    if (ret < 0)
        log_err("close pipe from main, 1 in client disconnecting: %s", strerror(errno));
    *clients_count -= 1;
    if (*clients_count != position) //not last elem of array, move last elem to empty cell
    {
        memcpy(clients + position, clients + *clients_count, sizeof(client_info));
        clients[*clients_count].client_id = 0;
    }
}

//if sigchild received set flag = 1
void childsignal(int signal)
{
    sigchld_received = 1;
}

//if sigint received from master
void slave_end(int signal)
{
    killpg(0, SIGINT);
    exit(0);
}

//ingore function for signal()
void ignore(int signal){}

//slave for UDP
int slave(int sk, struct sockaddr_in* name, int pipe_from_master, 
          int (*handler)(char* buffer, int* shellfd))
{
    int buf_pipe[2];
    pipe(buf_pipe);

    int shellfd = -1;

    int ret = dup2(buf_pipe[1], STDOUT_FILENO);
    if (ret < 0)
        log_err("dup2 in starting subprocess, stdout");
    ret = dup2(buf_pipe[1], STDERR_FILENO);
    if (ret < 0)
        log_err("dup2 in starting subprocess, stderr");

    int flag = 1;
    while (flag){
        char data[BUFSZ] = {0};
        int ret = read(pipe_from_master, data, BUFSZ);
        if (ret < 0){
            log_err("read in subprocess from main pipe");
            continue;
        }
        flag = handler(data, &shellfd);
        send_data(sk, name, data, buf_pipe[0], send_buf);
        send_buf(sk, name, "");
    }
}

//master for UDP
int master(int sk, struct sockaddr_in* name, 
           int (*handler)(char* buffer, int* shellfd))
{
    int pipe_from_master = 0;
    char data[BUFSZ] = {0};
    char buffer[BUFSZ] = {0};
    client_info* clients = (client_info*)malloc(MAX_CLIENTS_COUNT * sizeof(client_info));
    int flag = 1, clients_count = 0, ret, position, command;

    signal(SIGCHLD, childsignal);

    while (flag){
        receive_buf(sk, name, buffer);

        int client_id = separate_buffer(buffer, data);
        position = find_client(clients, client_id, clients_count);
        if (position < 0){
            ret = add_client(clients, client_id, clients_count);
            if (ret < 0)
                continue;

            position = clients_count;
            clients_count++;

            pipe_from_master = clients[position].pipes_from_main[0];
            int pid = fork();
            if (pid == 0){
                free(clients);
                signal(SIGCHLD, ignore);
                signal(SIGINT, slave_end);
                slave(sk, name, pipe_from_master, handler);
                return 0;
            }
                    
            log_init("server.log");
            log_info("id: %d\tConnected", clients[position].client_id); 
            clients[position].pid = pid;
        }
    
        command = get_command(data);
        switch (command){
            case findall:
                log_info("FINDALL");
                snprintf(data, BUFSZ, "%s", "server");
                send_buf(sk, name, data);
                continue;
            default:
                ret = write(clients[position].pipes_from_main[1], data, strlen(data));
                if (ret < 0){
                    log_err("write in pipe to subprocess: %s", strerror(errno));
                    continue;
                }
        }

        switch (command){
            case exit_com:
                if (clients[position].shell){
                    clients[position].shell = 0;
                    log_info("id: %d\tDeactivated shell", clients[position].client_id);
                }
                else{
                    log_info("id: %d\tClient disconnected", clients[position].client_id);
                    client_disconnect(clients, position, &clients_count);  //pipes will be killed before ending of slave
                }
                break;
            case shell:
                clients[position].shell = 1;
                log_info("id: %d\tStarting shell", clients[position].client_id);
                break;
            default:
                log_info("Id: %d\tCommand: %s", clients[position].client_id, data);
        }
    }

    free(clients);
    return 1;
}
