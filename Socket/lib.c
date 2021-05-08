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
    struct pollfd poll_info = {fd, POLLIN};

    while ((ret = poll(&poll_info, 1, 2 * TIMEOUT)) > 0) {
        clear_buf(data, BUFSZ);
        ret = read(fd, data, BUFSZ);
        if (ret < 0){
            log_err("read from fd in send_data");
            continue;
        }
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

void start_daemon()
{
    daemon(1, 1);
}

void* choose_protocol(char* type)
{
    if (starts_with(type, UDP)) {
        log_info("Choose UDP protocol");
        return dlopen("./lib_UDP.so", RTLD_LAZY);
    }
    else if (starts_with(type, TCP)){
        log_info("Choose TCP protocol");
        return dlopen("./lib_TCP.so", RTLD_LAZY);
    }
    else {
        log_err("argument should be tcp or udp");
        return NULL;
    }
}

functions get_functions(char* type)
{
    functions functions;
    functions.success = 1;
    void* sl = choose_protocol(type);
    if (sl == NULL){
        log_err("Bad shared library pointer");
        functions.success = error;
        return functions;
    }
    functions.create_socket = dlsym(sl, "create_socket");
    functions.listen_socket = dlsym(sl, "listen_socket");
    functions.master = dlsym(sl, "master");
    functions.receive_buf = dlsym(sl, "receive_buf");
    functions.send_buf = dlsym(sl, "send_buf");
    functions.connect_socket = dlsym(sl, "connect_socket");
    if ((functions.create_socket == NULL)||(functions.listen_socket == NULL)||
        (functions.send_buf == NULL)||(functions.master == NULL)||
        (functions.receive_buf == NULL)||(functions.connect_socket == NULL)){
        log_err("functions not found:\ncreate_socket: %p\nlisten_socket: %p\nconnect_socket: %p", 
        functions.create_socket, functions.listen_socket, functions.connect_socket);
        log_err("send_buf: %p\nmaster: %p\nreceive_buf: %p", 
        functions.send_buf, functions.master, functions.receive_buf);
        functions.success = error;
    }
    return functions;
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
    else if (starts_with(buffer, QUIT))
        return quit;
    else 
        return unknown;
}