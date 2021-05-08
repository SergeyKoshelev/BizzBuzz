#include "lib.h"
#include "log.h"

char client_id[IDSZ] = {0};

//get exclusive id for recognizing in server
int get_id()
{
    srand(time(NULL));
    int id = rand();  //random number between 0 and RAND_MAX
    char tmp_id[IDSZ] = {0};
    int ret = snprintf(tmp_id, IDSZ, "%d", id);
    if (ret < 0){
        log_err("sprintf in get_id");
        return -1;
    }
    memset(client_id, '0', IDSZ);
    ret = snprintf(client_id + IDSZ - strlen(tmp_id), IDSZ, "%s", tmp_id);
    if (ret < 0){
        log_err("sprintf in get_id");
        return -1;
    }
    return 0;
}

int get_str(char* buffer)
{
    char tmp_buf[BUFSZ - IDSZ] = {0};
    char* ret_s = fgets(tmp_buf, BUFSZ - IDSZ, stdin);
    if (ret_s == NULL){
        //log_err("fgets in get_str: %s", strerror(errno));
        return error;
    }
    
    int ret = snprintf(buffer, BUFSZ, "%s%s", client_id, tmp_buf);
    if (ret < 0){
        log_err("sprintf in get_str");
        return error;
    }
    return get_command(tmp_buf);
}

int signal_handling()
{
    struct termios termios_p;
    int ret = tcgetattr(STDIN_FILENO, &termios_p);
    if (ret < 0){
        log_err("cant get attr: %s", strerror(errno));
    }
    else
    {
        termios_p.c_lflag &= ~(ISIG);
        ret = tcsetattr(STDIN_FILENO, TCSANOW, &termios_p);
        if (ret < 0)
            log_err("cant set attr: %s", strerror(errno));
    }
}

int findall_handler(int sk, char* buffer, struct sockaddr_in* name, functions functions)
{
    int ret = setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)); //set broadcast mode
    if (ret < 0){
        log_err("setsockopt can't change mode");
        return error;
    }
    name->sin_addr.s_addr = htonl(INADDR_BROADCAST);   //make broadcast
    functions.send_buf(sk, name, buffer);

    functions.receive_buf(sk, name, buffer);
    printf("find server, buf: (%s), ip: %s\n", buffer, inet_ntoa(name->sin_addr));
    ret = setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &(int){0}, sizeof(int)); //return to usual mode
    if (ret < 0)
        log_err("setsockopt can't change mode");
}

int main(int argc, char** argv) {
    if (argc < 3){
        printf("Not enough arguments. Should be ./server [protocol] [ip]\n");
        return -1;
    }
    struct sockaddr_in name = {0};
    struct in_addr addr = {0};
    int sk, ret, command, flag = 1;
    char buffer[BUFSZ] = {0};
    log_init(NULL);
    functions functions = get_functions(argv[1]);
    if (functions.success == error){
        log_err("error with getting shared library functions");
        return -1;             
    }
    convert_address(argv[2], &addr);
    create_sock_name(&name, addr);
    if ((sk = functions.create_socket()) < 0){
        log_err("Can't create socket");
        return -1;
    }
    if ((ret = functions.connect_socket(sk, name)) < 0){
        log_err("Cant connect socket");
        return -1;
    }
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = 0;
    if ((starts_with(argv[1], UDP)) && ((ret = bind_socket(sk, name)) < 0)){
        log_err("Cant bind socket: %s", strerror(errno));
        return -1;
    }
    if ((ret = get_id()) < 0){
        log_err("Cant create exclusive id");
        return -1;
    }
    //signal_handling();
    while (flag){
        create_sock_name(&name, addr);
        printf("type in your request: ");
        command = get_str(buffer);
        switch (command){
            case error:
                continue;
            case findall:
                findall_handler(sk, buffer, &name, functions);
                break;
            default:
                functions.send_buf(sk, &name, buffer);
                printf("response on request:\n");
                receive_data(sk, &name, buffer, functions.receive_buf);  
                if (command == quit)
                    flag = 0;
        }
    }
    printf("End of client\n");
}