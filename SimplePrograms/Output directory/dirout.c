#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>

int main (int argc, char** argv)
{
    assert (argc > 1);
    char* path = argv[1];
    DIR* dir = opendir(path);
    assert (dir != NULL);
    struct dirent* file;
    while (file = readdir(dir))
        printf("%s\n", file->d_name);
}