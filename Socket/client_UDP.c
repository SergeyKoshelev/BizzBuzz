#include "UDP.h"

//get exclusive id for recognizing in server
int get_id()
{
    return getpid(); //for local
    //return get_my_ip() - for internet 
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

int get_str(char* buffer, struct sockaddr_in* name)
{
    char tmp_buf[BUFSZ] = {0};
    fgets(tmp_buf, BUFSZ, stdin);
    tmp_buf[strlen(tmp_buf) - 1] = '\0';

    sprintf(buffer, "%d %s", get_id(), tmp_buf);
    
    if (!strncmp(tmp_buf, FINDALL, sizeof(FINDALL) - 1)) //if command findall
        return -1;
    else if (!strncmp(tmp_buf, EXIT, sizeof(EXIT) - 1))
        return 0;
    else
        return 1;
    
}

int main(int argc, char** argv) {

    assert(argc > 1);
    struct sockaddr_in name = {0};
    struct in_addr addr = {0};
    int sk, ret, flag = 1;
    char buffer[BUFSZ] = {0};
    
    convert_address(argv[1], &addr);
    sk = create_socket(); 

    while (flag)
    {
        create_sock_name(&name, addr);
        printf("type in your request: ");
        flag = get_str(buffer, &name);
        //printf("flag: (%d)\tbuffer:(%s)\n", flag, buffer);
        
        if (flag >= 0) //usual command
        {
            ret = sendto(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, sizeof(name));
            if (ret < 0)
            {
                perror("Unable to write");
                exit(1);
            }

            name.sin_addr.s_addr = htonl(INADDR_ANY);
            name.sin_port = 0;
            bind_socket(sk, name);  //bind to rec message back
            
            if (flag > 0) //if command not exit
            {
                socklen_t fromlen = sizeof(name);
                ret = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, &fromlen);
                if (ret < 0 || ret > BUFSZ)
                {
                    printf("Unexpected read error or overflow %d\n", ret);
                    exit(1);
                }

                printf("response on request:\n%s\n", buffer);
            }
        }
        else if (flag == -1) //findall command
        {
            ret = setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int));
            if (ret < 0)
            {
                perror("Can't change mode\n");
                exit(1);
            }

            struct in_addr addr_tmp = {INADDR_BROADCAST};
            create_sock_name(&name, addr_tmp);   //make broadcast
            ret = sendto(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, sizeof(name)); //send findall command
            if (ret < 0)
            {
                perror("Unable to write");
                exit(1);
            }

            name.sin_addr.s_addr = htonl(INADDR_ANY);
            name.sin_port = 0;
            bind_socket(sk, name);  //bind to rec message back

            socklen_t fromlen = sizeof(name);
            ret = recvfrom(sk, buffer, BUFSZ, 0, (struct sockaddr*)&name, &fromlen);
            if (ret < 0 || ret > BUFSZ)
            {
                printf("Unexpected read error or overflow %d\n", ret);
                exit(1);
            }

            printf("find server, buf: (%s), id: %s\n", buffer, inet_ntoa(name.sin_addr));
            //setsockopt(sk, SOL_SOCKET, SO_)  //change sk opt to usual mode (not broadcast)
        }
    }

    close(sk);
}