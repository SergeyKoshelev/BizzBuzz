#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

const size_t fdsize = 10000;

int main(int argc, char** argv)
{
    int count = argc - 1;
    assert(count > 0);
    struct pollfd* pollfds = (struct pollfd*)malloc(count * sizeof(struct pollfd));
    for (int i = 0; i < count; i++)
    {
        pollfds[i].fd = open(argv[i + 1], O_NONBLOCK | O_RDONLY);
        assert (pollfds[i].fd > 0 && "No such fifo");
        pollfds[i].events = POLLIN;
        pollfds[i].revents = 0;
    }

    //struct stat fd_stat;
    int respoll;
    int resread;
    char buf[fdsize];

    while (respoll = poll(pollfds, count, -1))
    {
    for (int i = 0; i < count; i++)
        if (pollfds[i].revents & POLLIN)
        {
            //fstat(pollfds[i].fd, &fd_stat);
            resread = read(pollfds[i].fd, buf, fdsize);
            if (resread > 0)
            {
                buf[resread] = '\0';
                printf("fifo: %s\tread: %s", argv[i + 1], buf);
            }
        }
    }
    free(buf);
    free(pollfds);
}