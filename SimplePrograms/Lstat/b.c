#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
    assert(argc > 1);
    char* path = argv[1];
    struct stat stat;
    while (1)
    {
    int res = lstat(path, &stat);
	if (res == -1)
         printf("nety\n");
    else
        printf("est\n");
    scanf("%d", res);
    }
}
