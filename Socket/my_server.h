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
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <termios.h>
#include <poll.h>

#define PATH "/tmp/mysock"
#define BUFSZ 256
#define port 23456

#define DUMMY_STR "lalala printing smth for otl10"

#define PRINT "print"
#define EXIT "exit"
#define CD "cd"
#define LS "ls"
#define CLOSE "close"

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

int get_str(char* buffer, struct sockaddr_in* name)
{
    char tmp_buf[BUFSZ] = {0};
    char* ret_s = fgets(tmp_buf, BUFSZ, stdin);
    if (ret_s == NULL)
        perror("fgets in get_str");
    tmp_buf[strlen(tmp_buf) - 1] = '\0';
    //int ret = sprintf(buffer, "%d %s", get_id(), tmp_buf);
    int ret = sprintf(buffer, "%s", tmp_buf);
    if (ret < 0)
        perror("sprintf in get_str");
    
    /*
    if (starts_with(tmp_buf, FINDALL)) //if command findall
        return -1;
    else if (starts_with(tmp_buf, EXIT))
        return 0;
    else if (starts_with(tmp_buf, SHELL))
        return 2;
    else
        return 1;
    */
    
}


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

void bind_socket (int sk, struct sockaddr_in name)
{
    int ret = bind(sk, (struct sockaddr*)&name, sizeof(name));
    if (ret < 0)
    {
        perror("Unable to bind socket");
        close(sk);
        exit(1);
    }
}

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

void clear_buf(char* buffer, int size)
{
    for (int i = 0; i < size; i++)
        buffer[i] = 0;
}

#endif