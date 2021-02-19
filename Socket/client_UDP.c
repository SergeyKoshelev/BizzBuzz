#include "my_server.h"

#include <arpa/inet.h>
#include <assert.h>
const int port = 23456;
const char ip[] = "127.0.0.1";

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

void create_sock_name(struct sockaddr_in* name, char* serv_ip)
{
    name->sin_family = AF_INET;
    name->sin_port = htons(23456);
    int ret = inet_aton(serv_ip, &name->sin_addr);
    if (ret == 0)
    {
        perror("Invalid address");
        exit(1);
    }
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

int main(int argc, char** argv) {
    //htonl() for port=htons(10000)  and  ip = htonl(...)
    //inet_addr for convert ip in addr_type
    //my ip INADDR_LOOPBACK
    //use ports > 20000

    assert(argc > 1);
    struct sockaddr_in name = {0};
    int sk, ret;
    char buffer[BUFSZ] = {0};
    
    create_sock_name(&name, argv[1]);
    sk = create_socket();
    connect_socket(sk, name);
    fgets(buffer, BUFSZ, stdin);
    buffer[strlen(buffer) - 1] = '\0';

    while (strncmp(buffer, CLOSE, sizeof(CLOSE) - 1))
    {
        ret = sendto(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, sizeof(name));
        if (ret < 0)
        {
            perror("Unable to write");
            exit(1);
        }

        fgets(buffer, BUFSZ, stdin);
        buffer[strlen(buffer) - 1] = '\0';
    }

    close(sk);
}