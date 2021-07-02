#include <stdio.h>
#include <stdlib.h>

const char* name = "MY_VAR";

int main()
{
    char* s = getenv("name");
    if (s)
        printf("%s\n", s);
    else
        printf("Not found\n");   
}