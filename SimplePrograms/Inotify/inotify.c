#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>


int main(int argc, char** argv)
{
    int in, watch, ret;
    int mask = IN_CREATE | IN_DELETE | IN_MOVE_SELF;
    char buf[4096];
    struct inotify_event* event; 

    assert(argc > 1);

    in = inotify_init();
    assert (in >= 0);

    watch = inotify_add_watch(in, argv[1], mask);
    assert (watch >= 0);
    
    while (1)
    {
        ret = read(in, buf, 4096);
        if (ret > 0)
        {
            event = buf;
            printf("path: %s  ", event->name);

            if (event->mask & IN_CREATE)
                printf("event: create\n");
            else if (event->mask & IN_DELETE)
                printf("event: delete\n");
            else if (event->mask & IN_MOVE_SELF)
                printf("event: move\n");
            else
                printf("UNKNOWN\n");
        }
    }
}