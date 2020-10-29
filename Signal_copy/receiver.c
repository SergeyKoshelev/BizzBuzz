#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>
#include <signal.h>

const int byte = 8;
int arr_len = 0;
int *arr = NULL;
int flag = 1;

int my_open (int argc, char** argv)
{
	if (argc == 2)
	{
         int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
		 if (fd <= 0)
		 {
			printf("Can't open file\n");
            exit(1);
		 }
         return fd;
	}
	else
	{   
		printf("Incorrect count of arguments\n");
		exit(0);
	}
}

void get_0()
{ 
    arr[arr_len] = 0;
    arr_len++;
}
void get_1()
{ 
    arr[arr_len] = 1;
    arr_len++;
}
void finish ()
{ 
    flag = 0;
}

void write_symbol (int fd)
{
    int num = 0;
    int pov_2 = 1;
    for (int i = 0; i < byte; i++)
    {
        num += arr[i] * pov_2;
        pov_2 *= 2;
    }
    char sym = (char)num;
    write(fd, &sym, 1);
}

sigset_t* create_sigset ()
{
    sigset_t* sigset = (sigset_t*)malloc(sizeof(sigset_t));
    sigemptyset(sigset);
    sigaddset(sigset, SIGUSR1);
    sigaddset(sigset, SIGUSR2);
    sigaddset(sigset, SIGINT);
    return sigset;
}

int main(int argc, char** argv)
{
    int fd = my_open(argc, argv);

    pid_t pid = getpid();
    printf("My pid: %d\n", pid);

    arr = (int*)malloc(byte * sizeof(int));
    
    sigset_t* sigset = create_sigset();
    siginfo_t* siginfo = (siginfo_t*)malloc(sizeof(siginfo_t));
    sigprocmask(SIG_BLOCK, sigset, NULL);

    printf("Waiting for data...\n");

    int signal;
    while(flag)
    {  
        signal = sigwaitinfo(sigset, siginfo);
        if (signal == SIGUSR1) get_0();
        else if (signal == SIGUSR2) get_1();
        else if (signal == SIGINT) finish();

        if (arr_len == 8)
        {
            write_symbol(fd);
            arr_len = 0;
        }

        kill(siginfo->si_pid, SIGUSR1);
    }  

    printf("Finished!\n");

    free(arr);
    close(fd);
    free(sigset);
    free(siginfo);   
}