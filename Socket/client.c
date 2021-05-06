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
    if (ret < 0)
    {
        log_err("sprintf in get_id");
        return -1;
    }
    memset(client_id, '0', IDSZ);
    ret = snprintf(client_id + IDSZ - strlen(tmp_id), IDSZ, "%s", tmp_id);
    if (ret < 0)
    {
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
        //exit(0);
        return error;
    }
    //tmp_buf[strlen(tmp_buf) - 1] = '\0';
    
    int ret = snprintf(buffer, BUFSZ, "%s%s", client_id, tmp_buf);
    if (ret < 0){
        log_err("sprintf in get_str");
        return error;
    }
    
    //printf("(%s)\n", buffer);
    return get_command(tmp_buf);
}

int main(int argc, char** argv) {
    if (argc < 3)
    {
        printf("Not enough arguments. Should be ./server [protocol] [ip]\n");
        return -1;
    }
    struct sockaddr_in name = {0};
    struct in_addr addr = {0};
    int sk, ret, command, flag = 1, shell_activated;
    char buffer[BUFSZ] = {0};
    
    log_init(NULL);
    void* sl = choose_protocol(argv[1]);
    if (sl == NULL){
        log_err("Bad shared library pointer");
        return -1;
    }
    int (*create_socket)() = dlsym(sl, "create_socket");
    int (*receive_buf)(int sk, struct sockaddr_in* name, char* buffer) = dlsym(sl, "receive_buf");
    int (*send_buf)(int sk, struct sockaddr_in* name, char* data) = dlsym(sl, "send_buf");
    int (*connect_socket) (int sk, struct sockaddr_in name) = dlsym(sl, "connect_socket");
    
    convert_address(argv[2], &addr);
    create_sock_name(&name, addr);
    sk = create_socket();
    if (sk < 0){
        log_err("Can't create socket");
        return -1;
    }
    ret = connect_socket(sk, name);
    if (ret < 0){
        log_err("Cant connect socket");
        return -1;
    }

    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = 0;
    if (starts_with(argv[1], UDP)){
        ret = bind_socket(sk, name); //remove for tcp
        if (ret < 0){
            log_err("Cant bind socket: %s", strerror(errno));
            return -1;
        }
    }
    
    ret = get_id();
    if (ret < 0){
        log_err("Cant create exclusive id");
        return -1;
    }

    /*
    struct termios termios_p;
    ret = tcgetattr(STDIN_FILENO, &termios_p);
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
    */

    while (flag)
    {
        create_sock_name(&name, addr);
        printf("type in your request: ");
        command = get_str(buffer);
        //printf("flag: (%d)\tbuffer:(%s)\n", flag, buffer);
        
        switch (command){
            case error:
                continue;
            case findall:
                ret = setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)); //set broadcast mode
                if (ret < 0){
                    log_err("setsockopt can't change mode");
                    continue;
                }
                name.sin_addr.s_addr = htonl(INADDR_BROADCAST);   //make broadcast
                send_buf(sk, &name, buffer);

                receive_buf(sk, &name, buffer);
                printf("find server, buf: (%s), ip: %s\n", buffer, inet_ntoa(name.sin_addr));
                ret = setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &(int){0}, sizeof(int)); //return to usual mode
                if (ret < 0)
                    log_err("setsockopt can't change mode");
                break;
            default:
                printf("(%s)", buffer);
                send_buf(sk, &name, buffer);
                printf("response on request:\n");
                receive_data(sk, &name, buffer, receive_buf);  
                switch (command){  //switch for exit_com to end client process
                    case shell:
                        shell_activated = 1;
                        break;
                    case exit_com:
                        if (shell_activated)
                            shell_activated = 0;
                        else
                            flag = 0;
                }
        }
    }

    printf("End of client\n");
}