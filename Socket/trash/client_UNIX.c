#include "my_server.h"

int create_socket()
{
    int sk = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sk < 0)
    {
        perror("Unable to create socket");
        exit(1);
    } 

    return sk;
}

void create_sock_name(struct sockaddr_un* name)
{
    name->sun_family = AF_UNIX;
    strncpy(name->sun_path, PATH, sizeof(PATH));
}

void connect_socket (int sk, struct sockaddr_un name)
{
    int ret = connect(sk, (struct sockaddr*)&name, sizeof(name));
    if (ret)
    {
        perror("Unable to connect socket");
        close(sk);
        exit(1);
    }
}

int main() {
    struct sockaddr_un name = {0};
    int sk, ret, flag = 1;
    char buffer[BUFSZ] = {0};
    
    create_sock_name(&name);
    
    while (flag)
    {
        sk = create_socket();
        connect_socket(sk, name);
        scanf("%s", buffer);
        ret = write(sk, buffer, BUFSZ);
        if (ret < 0)
        {
            perror("Unable to write");
            exit(1);
        }
        close(sk);
    }
}