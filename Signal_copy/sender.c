#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

//sigset - set of signals
//siginfo - information about signal
//sigwaitinfo - waiting for one of the signals from set
//sigemptyset - create empty sigset
//sigaddset - add signal to the set
//sigprocmask - make signals from set blocked

const int byte = 8;
#define SIG_FINISH SIGTERM
#define SIG_GET0 SIGUSR1
#define SIG_GET1 SIGUSR2
#define SIG_CONTINUE SIGALRM
#define SIG_STOP SIGINT

siginfo_t* siginfo;

int my_open (int argc, char** argv)
{
    int fd;
	if (argc == 3)
	{
         fd = open(argv[1], O_RDONLY);
		 if (fd <= 0)
		 {
			 printf("No such file\n");
		 	exit(1);
		 }
	}
	 else
	{   
		printf("Incorrect count of arguments\n");
		exit(0);
	}
    return fd;
}

off_t file_size (int fd)
{
    struct stat fd_stat;
	fstat(fd, &fd_stat);
    if (fd_stat.st_size == 0) { printf("Empty file\n");}
	return fd_stat.st_size;
}

void get_bites (char* symbol, int* arr)
{   
    unsigned num = *(unsigned*)(symbol);
    
    unsigned mask = 1;
    for (int i = 0; i < byte; i++)
    {
        arr[i] = num & mask;
        num = num >> 1;
    }
}

int send_bites (pid_t rec_pid, int* arr, sigset_t* sigset)
{
    int signal; 
    for (int i = 0; i < byte; i++)
    {
        if (arr[i] == 0)
            kill(rec_pid, SIG_GET0);
        else
            kill(rec_pid, SIG_GET1);

        signal = sigwaitinfo(sigset, siginfo);
        if (siginfo->si_pid == rec_pid) 
        switch ( signal )
        {
            case SIG_FINISH :
                printf("Receiver made me finished\n");
                return SIG_FINISH;
                break;
            case SIG_CONTINUE :
                break;
            default:
                printf("Incorrect signal\n");
        }
        else if (signal == SIG_STOP)
        {
            printf("\n");
            kill (rec_pid, SIG_STOP);
            return SIG_STOP;
        }
    }
    return signal;
}

sigset_t* create_sigset ()
{
    sigset_t* sigset = (sigset_t*)malloc(sizeof(sigset_t));
    sigemptyset(sigset);
    sigaddset(sigset, SIG_FINISH);
    sigaddset(sigset, SIG_CONTINUE);
    sigaddset(sigset, SIG_STOP);
    return sigset;
}

int main(int argc, char** argv)
{
    pid_t rec_pid = atoi(argv[2]);
    pid_t my_pid = getpid();
    sigset_t* sigset = create_sigset();
    sigprocmask(SIG_BLOCK, sigset, NULL);

    kill(rec_pid, SIG_CONTINUE);
    int signal = sigwaitinfo(sigset, NULL);
    if (signal == SIG_FINISH)
    {
        printf("\nReceiver is busy, i stop! My pid was: %d\n", my_pid);
        free(sigset);
        exit(my_pid);
    }

    printf("My pid: %d\t", my_pid);

    int fd = my_open (argc, argv);
    off_t fd_size = file_size(fd);
    printf("File size: %ld bytes\n", fd_size);

    int* sym_in_bites = (int*)malloc(byte * sizeof(int));
    char* symbol = (char*)malloc(sizeof(char));
    siginfo = (siginfo_t*)malloc(sizeof(siginfo_t));

    printf("Sending data...\n");

    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);

    off_t i = 0;
    while ((i < fd_size) && (signal == SIG_CONTINUE))
    {
        read(fd, symbol, 1);
        get_bites(symbol, sym_in_bites);
        signal = send_bites(rec_pid, sym_in_bites, sigset);
        //if (i % 500 == 0) printf("Ready: %ld%%\r", (100*i)/ fd_size);
        i++;
    }

    kill(rec_pid, SIG_FINISH);
    printf("Finished!\n");

    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    double time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec)/(1000000000.0);
    printf("Send size: %ld bytes\n", i);
    printf("Full time: %.2lfs\n", time);
    printf("Speed: %.0lf bytes/s\n", i/time);

    close(fd);
    free(sym_in_bites);
    free(symbol);
    free(sigset);
    free(siginfo);
}