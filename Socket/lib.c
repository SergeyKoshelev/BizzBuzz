#include "lib.h"
#include "log.h"

void clear_buf(char* buffer, int size)
{
    memset(buffer, 0, size);
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
    if (ret == 0){
        log_err("Invalid address");
        return -1;
    }
    return 1;
}

int bind_socket (int sk, struct sockaddr_in name)
{
    int ret = bind(sk, (struct sockaddr*)&name, sizeof(name));
    if ((ret < 0) && (errno != EINVAL)){
        log_err("Unable to bind socket: %s", strerror(errno));
        close(sk);
        return -1;
    }
    return ret;
}

void send_data(int sk, struct sockaddr_in* name, 
               char* data, int fd,
               int (*send_buf)(int sk, struct sockaddr_in* name, char* data))
{
    int ret = 0;
    //printf("data: %s\n", data);
    struct pollfd poll_info = {fd, POLLIN};

    while ((ret = poll(&poll_info, 1, 2 * TIMEOUT)) > 0) {
        clear_buf(data, BUFSZ);
        ret = read(fd, data, BUFSZ);
        if (ret < 0){
            log_err("read from fd in send_data");
            continue;
        }

        //printf("data: %s\n", data);
        send_buf(sk, name, data);
    }
}

void receive_data(int sk, struct sockaddr_in* name, char* buffer,
                  int (*receive_buf)(int sk, struct sockaddr_in* name, char* buffer))
{
    int size = 1, ret;      
    while (size)
    {
        clear_buf(buffer, BUFSZ);

        size = receive_buf(sk, name, buffer);
        //printf("0: (%c)\t", buffer[0]);
        //printf("last: (%c)\n", buffer[size - 1]);
        //printf("(%s)\n", buffer);
        if (size > 0)
        {
            ret = write(STDOUT_FILENO, buffer, size);
            if (ret < 0)
                log_err("write in receive_data");
        }
    }
}

int starts_with(char* str, char* substr)
{
    return (!strncmp(str, substr, strlen(substr)));
}

//start daemon (doesn't write daemon's pid)
void start_daemon()
{
    /*
    int pid = fork();
    if (pid < 0)
        exit(-1);
    
    if (pid > 0) //parent
    {
        sleep(1);
        exit(0);
    }
    
    if (setsid() < 0)
        exit(-1);
    
    signal(...);

    pid = fork();
    if (pid < 0)
        exit(-1);
    
    if (pid > 0) //parent
    {
        sleep(1);
        exit(0);
    }

    umask(...);
    chdir(...);
    
    int fd;
    for (fd = sysconf(_SC_OPEN_MAX); fd >=0; fd--)
        close(fd);log_info("%s", buffer);

    */
    daemon(1, 1);
}

void* choose_protocol(char* type)
{
    if (starts_with(type, UDP)) 
    {
        log_info("Choose UDP protocol");
        return dlopen("./lib_UDP.so", RTLD_LAZY);
    }
    else if (starts_with(type, TCP))
    {
        log_info("Choose TCP protocol");
        return dlopen("./lib_TCP.so", RTLD_LAZY);
    }
    else 
    {
        perror("argument should be tcp or udp");
        return NULL;
    }
}

int separate_buffer(char * buffer, char* data) 
{
    char client_id_str[IDSZ] = {0};
    strncpy(client_id_str, buffer, IDSZ);
    int client_id = atoi(client_id_str);
    strcpy(data, buffer + IDSZ);
    //log_info("(%d) (%s)", client_id, data);
    return client_id;
}

int encode(char* buffer)
{
    int size = strlen(buffer);
    for (int i = 0; i < size; i++)
        buffer[i] ^= CR_MASK;
}

int decode(char* buffer)
{
    encode(buffer);
}

int get_command(char* buffer)
{
    if (starts_with(buffer, PRINT))
        return print;
    else if (starts_with(buffer, EXIT))
        return exit_com;
    else if (starts_with(buffer, CD))
        return cd;
    else if (starts_with(buffer, LS))
        return ls;
    else if (starts_with(buffer, FINDALL))
        return findall;
    else if (starts_with(buffer, SHELL))
        return shell;
    else 
        return unknown;
}