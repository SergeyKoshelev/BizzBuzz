//htonl() for port=htons(10000)  and  ip = htonl(...)
//inet_addr for convert ip in addr_type
//my ip INADDR_LOOPBACK
//use ports > 20000

#ifndef _myserver_h
#define _myserver_h

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

#define PATH "/tmp/mysock"
#define BUFSZ 256
#define port 23456
#define MAX_CLIENTS_COUNT 10

#define DUMMY_STR "lalala printing smth for otl10"

#define PRINT "print"
#define EXIT "exit"
#define CD "cd"
#define LS "ls"
#define FINDALL "findall"

//#define ERR (code) (if (code < 0) {perror("ERROR"); exit(1)})

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

void send_data(int sk, struct sockaddr_in* name, char* data)
{
    int ret = sendto(sk, data, BUFSZ, 0, (struct sockaddr*)name, sizeof(*name)); //send findall command
    if (ret < 0)
    {
        perror("Unable to write");
        exit(1);
    }
}

void receive_data(int sk, struct sockaddr_in* name, char* buffer)
{
    socklen_t fromlen = sizeof(struct sockaddr_in);
    int ret = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)name, &fromlen);

    if (ret < 0 || ret > BUFSZ)
    {
        printf("Unexpected read error or overflow %d\n", ret);
        exit(1);
    }
}

int starts_with(char* str, char* substr)
{
    return (!strncmp(str, substr, strlen(substr)));
}
#endif