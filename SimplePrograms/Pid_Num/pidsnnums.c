#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main()
{
	int count = 3;
	int* pointer = NULL;
	int pid = getpid();
	int ppid = getppid();
	int* number = malloc (sizeof(int));
	*number = 1; 
//	int main_par_pid = pid;
//	printf ("PID = %d    PPID = %d    NUMBER = %d\n", pid, ppid, pid-main_par_pid);
	printf ("PID = %d    PPID = %d    NUMBER = %d\n", pid, ppid, *number);
	for (int i = 0; i < count; i++)
	{
		int temp_pid = fork();
		if (temp_pid == 0)
		{
			pid = getpid();
			ppid = getppid();
		//	printf ("PID = %d    PPID = %d    NUMBER = %d\n", pid, ppid, pid-main_par_pid);
			*number++;
			printf ("PID = %d    PPID = %d    NUMBER = %d\n", pid, ppid, *number);				
		}
		else 
		{
		}
		wait(pointer);
	}
	return 0;
}
