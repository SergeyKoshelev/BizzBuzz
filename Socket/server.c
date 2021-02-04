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

void bind_socket (int sk, struct sockaddr_un name)
{
    int ret = bind(sk, (struct sockaddr*)&name, sizeof(name));
    if (ret < 0)
    {
        perror("Unable to bind socket");
        close(sk);
        exit(1);
    }
}

void listen_socket (int sk, int count)
{
    int ret = listen(sk, count);
    if (ret)
    {
        perror("Unable to listen socket");
        close(sk);
        exit(1);
    }
}

int accept_socket (int sk)
{
    int client_sk = accept(sk, NULL, NULL); //if not NULL - get inf about connected client
    if (client_sk < 0)
    {
        perror("Unable to accept");
        exit(1);
    }
    return client_sk;
}

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
        printf("UNKNOWN COMMAND\n");
        return 0;
    }
    
}

int main()
{
    unlink(PATH);

    int sk, ret, flag = 1;
    struct sockaddr_un name = {0};

    sk = create_socket();
    create_sock_name(&name);
    bind_socket(sk, name);
    listen_socket(sk, 20);

    while(flag)
    {
        char buffer[BUFSZ] = {0};
        int client_sk = accept_socket(sk);

        ret = read(client_sk, buffer, BUFSZ);
        if (ret < 0 || ret > BUFSZ)
        {
            printf("Unexpected read error or overflow %d\n", ret);
            exit(1);
        }

        flag = handler(buffer);
        close(client_sk);
    }

    printf("End of server\n");
    unlink(PATH);
}