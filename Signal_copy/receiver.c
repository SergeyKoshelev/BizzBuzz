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

#define SIG_FINISH SIGTERM
#define SIG_GET0 SIGUSR1
#define SIG_GET1 SIGUSR2
#define SIG_CONTINUE SIGALRM
#define SIG_STOP SIGINT

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
    //printf("get 0\n");
    arr[arr_len] = 0;
    arr_len++;
}
void get_1()
{ 
    //printf("get 1\n");
    arr[arr_len] = 1;
    arr_len++;
}
void finish ()
{ 
    //printf("get sigint\n");
    printf("Finished!\n");
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
    arr_len = 0;
}

sigset_t* create_sigset ()
{
    sigset_t* sigset = (sigset_t*)malloc(sizeof(sigset_t));
    sigemptyset(sigset);
    sigaddset(sigset, SIG_FINISH);
    sigaddset(sigset, SIG_GET0);
    sigaddset(sigset, SIG_GET1);
    sigaddset(sigset, SIG_CONTINUE);
    sigaddset(sigset, SIG_STOP);
    return sigset;
}

int main(int argc, char** argv)
{
    int fd = my_open(argc, argv);

    pid_t my_pid = getpid();
    printf("My pid: %d\n", my_pid);

    arr = (int*)malloc(byte * sizeof(int));
    
    sigset_t* sigset = create_sigset();
    siginfo_t* siginfo = (siginfo_t*)malloc(sizeof(siginfo_t));
    sigprocmask(SIG_BLOCK, sigset, NULL);

    int signal = sigwaitinfo(sigset, siginfo);
    pid_t sen_pid = siginfo->si_pid;
    printf("Main sender pid: %d\n", sen_pid);
    kill(sen_pid, SIG_CONTINUE);

    printf("Waiting for data...\n");

    while(flag)
    {
        kill(sen_pid, SIG_CONTINUE);
        signal = sigwaitinfo(sigset, siginfo);
        //printf("received signal %d\n", signal);

        if (sen_pid == siginfo->si_pid) 
        {
            switch ( signal ) 
            {
                case SIG_GET0 :  //get 0
                    get_0();
                    break;
                case SIG_GET1 :  //get 1
                    get_1();
                    break;
                case SIG_FINISH :  //end of file, finish properly
                    finish();
                    break;
                case SIG_STOP :  //sender was stopped
                    printf("Badly finished\n");
                    flag = 0;
                    break;
                default:
                    printf("bad signal\n");
            }

            if (arr_len == 8)
                write_symbol(fd);
        }
        else if (signal == SIG_STOP)
        {
            printf("\nFinished badly\n");
            kill(sen_pid, SIG_FINISH);
            flag = 0;
        }
        else if (siginfo->si_pid != my_pid)
        {
            kill(siginfo->si_pid, SIG_FINISH);
            printf("More than 1 sender, i kill it, its pid: %d\n", siginfo->si_pid);
        }
    }  

    free(arr);
    close(fd);
    free(sigset);
    free(siginfo);   
}