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

void send_bites (pid_t pid, int* arr, sigset_t* sigset)
{
    for (int i = 0; i < byte; i++)
    {
        if (arr[i] == 0)
            kill(pid, SIGUSR1);
        else
            kill(pid, SIGUSR2);
        sigwaitinfo(sigset, NULL);
    }
}

sigset_t* create_sigset ()
{
    sigset_t* sigset = (sigset_t*)malloc(sizeof(sigset_t));
    sigemptyset(sigset);
    sigaddset(sigset, SIGUSR1);
    return sigset;
}

int main(int argc, char** argv)
{
    int fd = my_open (argc, argv);
    pid_t rec_pid = atoi(argv[2]);
    off_t fd_size = file_size(fd);

    int* sym_in_bites = (int*)malloc(byte * sizeof(int));
    char* symbol = (char*)malloc(sizeof(char));

    sigset_t* sigset = create_sigset();
    sigprocmask(SIG_BLOCK, sigset, NULL);

    printf("Sending data...\n");

    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);

    for (off_t i = 0; i < fd_size; i++)
    {
        read(fd, symbol, 1);
        get_bites(symbol, sym_in_bites);
        send_bites(rec_pid, sym_in_bites, sigset);
        if (i % 100 == 0) printf("Ready: %ld%%\r", (100*i)/ fd_size);
    }

    kill(rec_pid, SIGINT);
    printf("\nFinished!\n");

    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    double time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec)/(1000000000.0);
    printf("File size: %ld bytes\n", fd_size);
    printf("Full time: %.2lfs\n", time);
    printf("Speed: %.0lf bytes/s\n", fd_size/time);

    close(fd);
    free(sym_in_bites);
    free(symbol);
    free(sigset);
}