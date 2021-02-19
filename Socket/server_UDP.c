#include "my_server.h"

#include <arpa/inet.h>
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

void bind_socket (int sk, struct sockaddr_in name)
{
    int ret = bind(sk, (struct sockaddr*)&name, sizeof(name));
    if (ret < 0)
    {
        perror("Unable to bind socket");
        close(sk);
        exit(1);
    }
}

void create_sock_name(struct sockaddr_in* name)
{
    name->sin_family = AF_INET;
    name->sin_port = htons(23456);
    name->sin_addr.s_addr = inet_addr(ip);
}

int handler (char* buffer)
{
    printf("buffer: (%s)\n", buffer);
    if (!strncmp(buffer, PRINT, sizeof(PRINT) - 1))
    {
        printf("%s\n", DUMMY_STR);
        return 1;
    }
    else if (!strncmp(buffer, EXIT, sizeof(EXIT) - 1))
    {
        return 0;
    }
    else if (!strncmp(buffer, LS, sizeof(LS) - 1))
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            int res = execlp("ls", "ls", NULL);
            printf("Error in exec in LS\n");
            exit(1);
        }
        waitpid(pid, NULL, 0);
        return 1;
    }
    else if (!strncmp(buffer, CD, sizeof(CD) - 1))
    {
        char* path = buffer + sizeof(CD);
        printf("new path: (%s)\n", path);
        int ret = chdir(path);
        if (ret < 0)
        {
            perror("Cant change directory\n");
            exit(1);
        }
        return 1;
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
    struct sockaddr_in name = {0};
    socklen_t fromlen = sizeof(name);

    sk = create_socket();
    create_sock_name(&name);
    bind_socket(sk, name);

    while(flag)
    {
        char buffer[BUFSZ] = {0};
        ret = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, &fromlen);

        if (ret < 0 || ret > BUFSZ)
        {
            printf("Unexpected read error or overflow %d\n", ret);
            exit(1);
        }

        flag = handler(buffer);
    }

    printf("End of server\n");
    unlink(PATH);
}