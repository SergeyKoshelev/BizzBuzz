#include "my_server.h"

#include <arpa/inet.h>
const char server_ip[] = "127.0.0.1";

#define COUNT_CLIENTS 2

int handler (char* buffer)
{
    if (!strncmp(buffer, PRINT, sizeof(PRINT)))
    {
        printf("%s\n", DUMMY_STR);
        return 1;
    }
    else if (!strncmp(buffer, EXIT, sizeof(EXIT)))
    {
        return 0;
    }
    else
    {
        printf("UNKNOWN COMMAND:\n(%s)\n", buffer);
        //return 0;
        return 1;
    }
    
}

int main()
{
    unlink(PATH);

    int sk, ret, flag = 1;
    struct sockaddr_in name = {0};

    struct in_addr addr = {INADDR_ANY}; //for accepting all incoming messages, server_ip become useless
    sk = create_socket();

    convert_address(server_ip, &addr);
    create_sock_name(&name, addr);
    bind_socket(sk, name);
    listen_socket(sk, COUNT_CLIENTS);

    int client_sk = 0;

    while(flag)
    {
        client_sk = accept_socket(sk);
        char buffer[BUFSZ] = {0};
        ret = recvfrom(client_sk, buffer, BUFSZ, 0, NULL, NULL);
        if (ret < 0 || ret > BUFSZ)
        {
            printf("Unexpected read error or overflow %d\n", ret);
            exit(1);
        }
        printf("read: %s\n", buffer);
        buffer[0] = 'a';

        ret = send(client_sk, buffer, BUFSZ, 0);
        printf("ret: %d\n", ret);
        if (ret < 0)
        {
            perror("Unable to write");
            exit(1);
        }

        printf("write: %s\t\n", buffer);
        //close(client_sk);
    }
    

    close(sk);
    printf("End of server\n");
    unlink(PATH);
}