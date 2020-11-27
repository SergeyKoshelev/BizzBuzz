#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

const int arr_size = 100;

int main(int argc, char** argv)
{
    int in, watch;
    size_t count;
    int mask = IN_CREATE | IN_DELETE | IN_MOVE_SELF;
    struct inotify_event events[arr_size]; 

    assert(argc > 1);

    in = inotify_init();
    assert (in >= 0);
    
    
    watch = inotify_add_watch(in, argv[1], mask);
    assert (watch >= 0);
    
    while (1)
    {
        count = read(in, (void*)events, sizeof(struct inotify_event) * arr_size) / sizeof(struct inotify_event);
        for (int i = 0; i < count - 1; i++)
        {
            printf("%d   path: %s  ", i, events[i].name);

            if (events[i].mask & IN_CREATE)
                printf("event: create\n");
            else if (events[i].mask & IN_DELETE)
                printf("event: delete\n");
            else if (events[i].mask & IN_MOVE_SELF)
                printf("event: move\n");
            else
                printf("UNKNOWN\n");
        }
        printf("\n");
    }
}