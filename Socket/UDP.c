#include "lib.h"
#include "log.h"

client_info clients[MAX_CLIENTS_COUNT];
int clients_count = 0;
int* shellfd_p = NULL;

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
int find_client(int client_id)
{
    for (int i = 0; i < clients_count; i++)
        if (clients[i].client_id == client_id)
            return i;
    return -1;
}

//add new client in array
int add_client(int client_id)
{
    if (clients_count >= MAX_CLIENTS_COUNT)
        return -1;

    clients[clients_count].client_id = client_id;
    int ret = pipe(clients[clients_count].pipes_from_master);
    if (ret < 0){
        log_err("creating pipes from main to subprocess for new client: %s", strerror(errno));
        return -1;
    }
    return clients_count;
}

//disconnect server, closes pipes and replace cell of client 
int client_disconnect(int position)
{
    clients[position].client_id = 0;
    int ret = close(clients[position].pipes_from_master[0]); //close input in pipe
    if (ret < 0)
        log_err("close pipe from main, 0 in client disconnecting: %s", strerror(errno));
	ret = close(clients[position].pipes_from_master[1]); //close output from pipe
    if (ret < 0)
        log_err("close pipe from main, 1 in client disconnecting: %s", strerror(errno));
    clients_count -= 1;
    if (clients_count != position){ //not last elem of array, move last elem to empty cell
        memcpy(clients + position, clients + clients_count, sizeof(client_info));
        clients[clients_count].client_id = 0;
    }
}

//if sigint received from master
void slave_end(int signal)
{
    killpg(0, SIGINT);
    exit(0);
}

//sigchld handler for slave
void sigchld_slave(int signal){
    //log_info("Received sigchld in slave");
    if (*shellfd_p > 0){
        log_info("Shell deactivated");
        *shellfd_p = -1;
    }
}

//slave for UDP
int slave(int sk, struct sockaddr_in* name, int pipe_from_master, 
          int (*handler)(char* buffer, int* shellfd))
{
    signal(SIGCHLD, sigchld_slave);
    signal(SIGINT, slave_end);
    log_init("server.log");
    int buf_pipe[2];
    pipe(buf_pipe);
    int shellfd = -1;
    shellfd_p = &shellfd;

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
    char data[BUFSZ] = {0}, buffer[BUFSZ] = {0};
    int flag = 1, ret, position, command;
    while (flag){
        receive_buf(sk, name, buffer);
        int client_id = separate_buffer(buffer, data);
        position = find_client(client_id);
        if (position < 0){
            ret = add_client(client_id);
            if (ret < 0)
                continue;
            position = clients_count;
            clients_count++;
            pipe_from_master = clients[position].pipes_from_master[0];
            int pid = fork();
            if (pid == 0){
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
                ret = write(clients[position].pipes_from_master[1], data, strlen(data));
                if (ret < 0){
                    log_err("write in pipe to subprocess: %s", strerror(errno));
                    continue;
                }
        }
        log_info("Id: %d\tCommand: %s", clients[position].client_id, data);
        if (command == quit){
            log_info("Client disconnecting, id: %d", clients[position].client_id);
            client_disconnect(position);
        }
    }
    return 1;
}