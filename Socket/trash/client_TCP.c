#include "my_server.h"

#include <arpa/inet.h>
#include <assert.h>
const char ip[] = "127.0.0.1";

int main(int argc, char** argv) {
    //htonl() for port=htons(10000)  and  ip = htonl(...)
    //inet_addr for convert ip in addr_type
    //my ip INADDR_LOOPBACK
    //use ports > 20000

    int flag = 1;
    assert(argc > 1);
    struct sockaddr_in name = {0};
    struct in_addr addr = {0};
    int sk, ret;
    char buffer[BUFSZ] = {0};
    
    convert_address(argv[1], &addr);
    create_sock_name(&name, addr);
    sk = create_socket();
    connect_socket(sk, name);


    while (flag)
    {
        clear_buf(buffer, BUFSZ);
        get_str(buffer, &name);
        ret = send(sk, buffer, BUFSZ, 0);
        if (ret < 0)
        {
            perror("Unable to write");
            exit(1);
        }

        clear_buf(buffer, BUFSZ);

        ret = recvfrom(sk, buffer, BUFSZ, 0, NULL, NULL);
        if (ret < 0 || ret > BUFSZ)
        {
            printf("Unexpected read error or overflow %d\n", ret);
            exit(1);
        }

        printf("response: %s\n", buffer);

        //flag = 0;
    }
    
    close(sk);
}