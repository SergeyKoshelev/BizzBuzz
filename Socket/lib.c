#include "lib.h"

void clear_buf(char* buffer, int size)
{
    for (int i = 0; i < size; i++)
        buffer[i] = 0;
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

void send_data(int sk, struct sockaddr_in* name, 
               char* data, int fd, int client_sk,
               void (*send_buf)(int sk, struct sockaddr_in* name, char* data, int client_sk))
{
    int not_end = 1, count = 0, ret = 0;
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

void receive_data(int sk, struct sockaddr_in* name, 
                  char* buffer, int client_sk,
                  int (*receive_buf)(int sk, struct sockaddr_in* name, char* buffer, int client_sk))
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

int starts_with(char* str, char* substr)
{
    return (!strncmp(str, substr, strlen(substr)));
}

//open log file
FILE* open_log_file()
{
    FILE* log_file_p = fopen("log.txt", "a");
    if (log_file_p == NULL) 
    {
        printf("Can't open logfile\n");
        exit(1);
    }

    return log_file_p;
    //return stdout;
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

void* choose_protocol(char* type, FILE* logfile)
{
    if (starts_with(type, UDP)) 
    {
        printf("Choose UDP protocol\n");
        if (logfile != NULL)
            fprintf(logfile, "Choose UDP protocol\n");
        return dlopen("./lib_UDP.so", RTLD_LAZY);
    }
    else if (starts_with(type, TCP))
    {
        printf("Choose TCP protocol\n");
        if (logfile != NULL)
            fprintf(logfile, "Choose TCP protocol\n");
        return dlopen("./lib_UDP.so", RTLD_LAZY);
    }
    else 
    {
        perror("argument should be tcp or udp");
        return NULL;
    }
}
