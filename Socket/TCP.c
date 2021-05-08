#include "lib.h"
#include "log.h"

int* shellfd_p = NULL;

//TCP function
//create AF_INET, SOCK_DGRAM socket
int create_socket()
{
    int sk = socket(AF_INET, SOCK_STREAM, 0);
    return sk;
}

//only size of BUFSZ
//can be used only for receive command in server and for receive "findall" request in client
//need only client_sk (reading from it), buffer as arguments
int receive_buf(int sk, struct sockaddr_in* name, char* buffer)
{
    clear_buf(buffer, BUFSZ);
    int size = recv(sk, buffer, BUFSZ - 1, 0);

    if (size < 0 || size >= BUFSZ){
        log_err("Receive_buf: %s\tSize:%d", strerror(errno), size);
        return -1;
    }
    decode(buffer);
    return strlen(buffer); //because size is usually BUFSZ - 1
}

//TCPfunction
//only size of BUFSZ
//can be used only to connect for "findall" and for sending command from client
//need only client_sk(write in it) and data
int send_buf(int sk, struct sockaddr_in* name, char* data)
{
    encode(data);
    int size = send(sk, data, BUFSZ, 0);
    if (size < 0){
        log_err("Unable to write");
        return -1;
    }
    return size;
}

//connecting to socket in client
int connect_socket (int sk, struct sockaddr_in name){
    return connect(sk, (struct sockaddr*)&name, sizeof(name));}

//start listening socket in server
int listen_socket (int sk, int count){ 
    return listen(sk, count);}

//accepting socket in server, return client_sk
int accept_socket (int sk)
{
    int client_sk = accept(sk, NULL, NULL); //if not NULL - get inf about connected client
    if (client_sk < 0){
        log_err("Unable to accept");
        return -1;
    }
    return client_sk;
}

//sigchld handler for slave
void sigchld_slave(int signal){
    //log_info("Received sigchld in slave");
    if (*shellfd_p > 0){
        log_info("Shell deactivated");
        *shellfd_p = -1;
    }
}

//slave for TCP
int slave(int client_sk, struct sockaddr_in* name, 
          int (*handler)(char* buffer, int* shellfd))
{
    signal(SIGCHLD, sigchld_slave);
    log_init("server.log");
    int buf_pipe[2];
    pipe(buf_pipe);
    int shellfd = -1, command, flag = 1;
    shellfd_p = &shellfd;
    int ret = dup2(buf_pipe[1], STDOUT_FILENO);
    if (ret < 0)
        log_err("dup2 in starting subprocess, stdout");
    ret = dup2(buf_pipe[1], STDERR_FILENO);
    if (ret < 0)
        log_err("dup2 in starting subprocess, stderr");

    while (flag)
    {
        char data[BUFSZ] = {0}, buffer[BUFSZ] = {0};
        ret = receive_buf(client_sk, NULL, buffer);      
        if (ret <= 0)
            continue;    
        int client_id = separate_buffer(buffer, data);
        log_info("Id: %d\t Command: %s", client_id, data);
        command = get_command(data);
        switch (command){
            case findall:
                snprintf(data, BUFSZ, "%s", "server");
                send_buf(client_sk, name, data);
                continue;
            default:
                flag = handler(data, &shellfd);
                send_data(client_sk, name, data, buf_pipe[0], send_buf);
                //receive_buf(client_sk, NULL, buffer);
        }   
    }
}

//master for TCP
int master(int sk, struct sockaddr_in* name, 
           int (*handler)(char* buffer, int* shellfd))
{
    int flag = 1;
    while (flag){
        int client_sk = accept_socket(sk);
        if (client_sk > 0){
            int pid = fork();
            if (pid == 0){
                log_init("server.log");
                slave(client_sk, name, handler);
                return 0;
            }
        }
    }
}
