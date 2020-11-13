#include <stdio.h>
#include <stdlib.h>

const char* name = "MY_VAR";

int main(int argc, char** argv, char** envp)
{
    for (int i = 0; envp[i]; i++)
    {
        printf("%s\n", envp[i]);
    } 
}