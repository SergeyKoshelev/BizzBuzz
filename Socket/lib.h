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
#include <dlfcn.h>
#include <time.h>

#define PATH "/tmp/mysock"
#define BUFSZ 256
#define port 23456
#define MAX_CLIENTS_COUNT 10
#define TIMEOUT 100
#define IDSZ 10
#define CR_MASK 0xAA 

#define DUMMY_STR "lalala printing smth for otl10"

#define PRINT "print"
#define EXIT "exit"
#define CD "cd"
#define LS "ls"
#define FINDALL "findall"
#define SHELL "shell"
#define UDP "udp"
#define TCP "tcp"

enum Commands {
    error = -1,
    unknown = 0,
    print = 1,
    exit_com = 2,
    cd = 3,
    ls = 4,
    findall = 5,
    shell = 6
};

struct client_info {
    int pid;
    int client_id;
    int pipes_from_main[2];
    int pipes_to_main[2];
    int shell; //flag if shell activated
    int client_sk; //client sk for tcp
}; typedef struct client_info client_info;

//set all cells in '\0'
void clear_buf(char* buffer, int size);

//create socket name "name" with "addr"
void create_sock_name(struct sockaddr_in* name, struct in_addr addr);

//convert char* ip in in_addr addr
int convert_address(const char* ip, struct in_addr* addr);

//bind
int bind_socket (int sk, struct sockaddr_in name);

//more than BUFSZ size
void send_data(int sk, struct sockaddr_in* name, 
               char* data, int fd,
               int (*send_buf)(int sk, struct sockaddr_in* name, char* data));

//more than BUFSZ size
//UDP: sk, name, buffer
//TCP: buffer, client_sk
void receive_data(int sk, struct sockaddr_in* name, char* buffer,
                  int (*receive_buf)(int sk, struct sockaddr_in* name, char* buffer));

//check if str starts with substr
int starts_with(char* str, char* substr);

//start daemon (doesn't write daemon's pid)
void start_daemon();

//choose UDP or TCP protocol
void* choose_protocol(char* type);

//separate received buffer on id and data
//returns id, data in variable data
int separate_buffer(char * buffer, char* data);

//encode buffer
int encode(char* buffer);

//decode buffer
int decode(char* buffer);

//get enum command from string
int get_command(char* buffer); 