//htonl() for port=htons(10000)  and  ip = htonl(...)
//inet_addr for convert ip in addr_type
//my ip INADDR_LOOPBACK
//use ports > 20000
#ifndef _UDP_h
#define _UDP_h
#define _XOPEN_SOURCE
#define _GNU_SOURCE

#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>

#define PATH "/tmp/mysock"
#define BUFSZ 256
#define port 23456
#define MAX_CLIENTS_COUNT 10
#define TIMEOUT 100

#define DUMMY_STR "lalala printing smth for otl10"

#define PRINT "print"
#define EXIT "exit"
#define CD "cd"
#define LS "ls"
#define FINDALL "findall"
#define SHELL "shell"

#endif

struct client_info {
    int pid;
    int client_id;
    int pipes_from_main[2];
    int pipes_to_main[2];
    int shell; //flag if shell activated
}; typedef struct client_info client_info;

//#define ERR (code) (if (code < 0) {perror("ERROR"); exit(1)})

void clear_buf(char* buffer, int size)
{
    for (int i = 0; i < size; i++)
        buffer[i] = 0;
}

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

void create_sock_name(struct sockaddr_in* name, struct in_addr addr)
{
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    name->sin_addr.s_addr = addr.s_addr;
}

int convert_address(const char* ip, struct in_addr* addr)
{
    int ret = inet_aton(ip, addr);
    if (ret == 0)
    {
        perror("Invalid address");
        exit(1);
    }
    return 1;
}

void bind_socket (int sk, struct sockaddr_in name)
{
    int ret = bind(sk, (struct sockaddr*)&name, sizeof(name));
    if ((ret < 0) && (errno != EINVAL))
    {
        perror("Unable to bind socket");
        close(sk);
        exit(1);
    }
}

//only size of BUFSZ
//can be used only to connect for "findall" and for sending command from client
void send_buf(int sk, struct sockaddr_in* name, char* data)
{
    int ret = sendto(sk, data, BUFSZ, 0, (struct sockaddr*)name, sizeof(*name)); //send findall command
    if (ret < 0)
    {
        perror("Unable to write");
        exit(1);
    }
}

//more than BUFSZ size
void send_data(int sk, struct sockaddr_in* name, char* data, client_info* clients, int position)
{
    int not_end = 1, count = 0, ret = 0;
    int fd = clients[position].pipes_to_main[0];

    struct pollfd poll_info = {fd, POLLIN};
    while (ret = poll(&poll_info, 1, 2 * TIMEOUT) != 0) 
    {
        clear_buf(data, BUFSZ);
        ret = read(fd, data, BUFSZ);
        if (ret < 0)
            perror("read from pipe");

        send_buf(sk, name, data);
    }

    send_buf(sk, name, "");
}

//only size of BUFSZ
//can be used only for receive command in server and for receive "findall" request in client
int receive_buf(int sk, struct sockaddr_in* name, char* buffer)
{
    socklen_t fromlen = sizeof(struct sockaddr_in);
    int size = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)name, &fromlen);
    buffer[BUFSZ] = '\0';

    if (size < 0 || size > BUFSZ)
    {
        printf("Unexpected read error or overflow %d\n", size);
        exit(1);
    }

    return strlen(buffer);
}

//more than BUFSZ size
void receive_data(int sk, struct sockaddr_in* name, char* buffer)
{
    int not_empty = 1;
        
    while (not_empty)
    {
        clear_buf(buffer, BUFSZ);

        int size = receive_buf(sk, name, buffer);
        if (size == 0)
            not_empty = 0;
        else
            write(STDOUT_FILENO, buffer, size);
    }
}

//check if str starts with substr
int starts_with(char* str, char* substr)
{
    return (!strncmp(str, substr, strlen(substr)));
}