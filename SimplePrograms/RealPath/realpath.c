#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    char s[] = "./backuper/../Seminars";
    char path[20];
    char* res;
    res = realpath(s, path);
    printf("res: (%s) (%s)\n", path, res);
}