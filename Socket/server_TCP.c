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

    int clients_sk[COUNT_CLIENTS];
    for (int i = 0; i < COUNT_CLIENTS; i++)
    {
        clients_sk[i] = accept_socket(sk);
    }

    //connect_socket(sk, name);

    while(flag)
    {
        char buffer[BUFSZ] = {0};
        for (int i = 0; i < COUNT_CLIENTS; i++)
        {
            ret = recvfrom(clients_sk[i], buffer, BUFSZ, 0, NULL, NULL);
            if (ret < 0 || ret > BUFSZ)
            {
                printf("Unexpected read error or overflow %d\n", ret);
                exit(1);
            }
            printf("%d read: %s\n", i, buffer);
            buffer[0] = 'a';

            ret = send(clients_sk[i], buffer, BUFSZ, 0);
            printf("ret: %d\n", ret);
            if (ret < 0)
            {
                perror("Unable to write");
                exit(1);
            }

            printf("write: %s\nflag %d\n", buffer, flag);
        }
    }

    close(sk);
    for (int i = 0; i < COUNT_CLIENTS; i++)
        close(clients_sk[i]);
    printf("End of server\n");
    unlink(PATH);
}