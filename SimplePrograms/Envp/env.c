#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

int main(int argc, char** argv)
{
    /*
    int j = 0;
    char* str;

    //printf("command: %s\n", argv[1]);

    for (int i = 2; i < argc; i++)
    {
        j = 0;
        str = argv[i];
        //printf("str: %s\n", str);
        char **my_envp = (char**)malloc(strlen(str));
        while ((my_envp[j] = strtok_r(str, "=", &str)))
	    {
            //printf("my_envp[%d]=%s\n", j, my_envp[j]);
    		j++; 
	    }
        setenv(my_envp[0], my_envp[1], 1);
        //char* result = getenv(my_envp[0]);
        //printf("result: %s\n", result);
        free(my_envp);
    }
    */

   
    int j = 0;
    char* str= argv[1];
    char **command = (char**)malloc(strlen(str));
	while ((command[j] = strtok_r(str, " ", &str)))
	{
        //printf("command[%d]=%s\n", j, command[j]);
		j++;
	}


    /*
    int check = execvp(command[0], command);
    assert(check > 0);
    */
    
    
   execve(command[0], command, argv + 2);
}
