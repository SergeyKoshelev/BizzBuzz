#include "lib.h"

//TCP function
//create AF_INET, SOCK_DGRAM socket
int create_socket()
{
    int sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk < 0)
    {
        perror("Unable to create socket");
        exit(1);
    } 

    return sk;
}

//only size of BUFSZ
//can be used only for receive command in server and for receive "findall" request in client
//need only client_sk (reading from it), buffer as arguments
int receive_buf(int sk, struct sockaddr_in* name, char* buffer, int client_sk)
{
    int size = read(client_sk, buffer, BUFSZ);
    buffer[BUFSZ] = '\0';

    if (size < 0 || size > BUFSZ)
        printf("Unexpected read error or overflow %d\n", size);
    
    return strlen(buffer);
}

//TCPfunction
//only size of BUFSZ
//can be used only to connect for "findall" and for sending command from client
//need only client_sk(write in it) and data
void send_buf(int sk, struct sockaddr_in* name, char* data, int client_sk)
{
    int size = write(client_sk, data, BUFSZ);
    if (size < 0)
    {
        perror("Unable to write");
        exit(1);
    }
}

//connecting to socket in client
void connect_socket (int sk, struct sockaddr_in name)
{
    int ret = connect(sk, (struct sockaddr*)&name, sizeof(name));
    if (ret)
    {
        perror("Unable to connect socket");
        close(sk);
        exit(1);
    }
}

//start listening socket in server
void listen_socket (int sk, int count)
{
    int ret = listen(sk, count);
    if (ret)
    {
        perror("Unable to listen socket");
        close(sk);
        exit(1);
    }
}

//accepting socket in server, return client_sk
int accept_socket (int sk)
{
    int client_sk = accept(sk, NULL, NULL); //if not NULL - get inf about connected client
    if (client_sk < 0)
    {
        perror("Unable to accept");
        exit(1);
    }
    return client_sk;
}

//master for UDP
int master(int sk, struct sockaddr_in* name, 
           char* buffer, char* data, client_info* clients, 
           int* clients_count, int* pipe_from_fd, 
           int* client_sk, FILE* logfile)
{
    *client_sk = accept_socket(sk);
    if (*client_sk > 0)
    {
        int pid = fork();
        if (pid == 0)
            return -1;
    }
}

//slave for TCP
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

        receive_buf(-1, NULL, data, client_sk);
        flag = handler(data, &shellfd); //do command
        send_data(sk, name, data, buf_pipe[0], -1, send_buf);
    }
}