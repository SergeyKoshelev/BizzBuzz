//#define _TCP
#define _UDP

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

const char log_file[] = "log.txt";

struct client_info {
    int pid;
    int client_id;
    int pipes_from_main[2];
    int pipes_to_main[2];
    int shell; //flag if shell activated
    int client_sk; //client sk for tcp
}; typedef struct client_info client_info;

#ifdef _UDP
#include "UDP.h"
#endif

#ifdef _TCP
#include "TCP.h"
#endif

//set all cells in '\0'
void clear_buf(char* buffer, int size)
{
    for (int i = 0; i < size; i++)
        buffer[i] = 0;
}

//create socket name "name" with "addr"
void create_sock_name(struct sockaddr_in* name, struct in_addr addr)
{
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    name->sin_addr.s_addr = addr.s_addr;
}

//convert char* ip in in_addr addr
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

//bind
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

//more than BUFSZ size
void send_data(int sk, struct sockaddr_in* name, char* data, client_info* clients, int position, int client_sk)
{
    int not_end = 1, count = 0, ret = 0;
    int fd = clients[position].pipes_to_main[0];
    //printf("data: %s\n", data);
    struct pollfd poll_info = {fd, POLLIN};

    while (ret = poll(&poll_info, 1, 2 * TIMEOUT) != 0) 
    {
        clear_buf(data, BUFSZ);
        ret = read(fd, data, BUFSZ);
        if (ret < 0)
            perror("read in send_data");

        //printf("data: %s\n", data);
        send_buf(sk, name, data, client_sk);
    }

    send_buf(sk, name, "", client_sk);
}

//more than BUFSZ size
//UDP: sk, name, buffer
//TCP: buffer, client_sk
void receive_data(int sk, struct sockaddr_in* name, char* buffer, int client_sk)
{
    int not_empty = 1;
        
    while (not_empty)
    {
        clear_buf(buffer, BUFSZ);

        int size = receive_buf(sk, name, buffer, client_sk);
        //printf("(%s)\n", buffer);
        if (size == 0)
            not_empty = 0;
        else
        {
            int ret = write(STDOUT_FILENO, buffer, size);
            if (ret < 0)
                perror("write in receive_data");
        }
    }
}

//check if str starts with substr
int starts_with(char* str, char* substr)
{
    return (!strncmp(str, substr, strlen(substr)));
}

//open log file
FILE* open_log_file()
{
    FILE* log_file = fopen("log.txt", "a");
    if (log_file == NULL) 
    {
        printf("Can't open logfile\n");
        exit(1);
    }

    return log_file;
}

//start daemon (doesn't write daemon's pid)
void start_daemon()
{
    int pid = fork();

    if (pid != 0) //parent
    {
        sleep(1);
        exit(0);
    }
}
