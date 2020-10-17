#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

int my_open (int argc, char** argv)
{
    int fd;
	if (argc == 2)
         fd = open(argv[1], O_RDONLY);
	 else
	{   
		printf("Incorrect count of arguments\n");
		exit(0);
	}
    return fd;
}

char* my_read(int fd)
{
  	struct stat fd_stat;
	fstat(fd, &fd_stat);
	off_t fd_size = fd_stat.st_size;
    char* buf = (char*)malloc(fd_size);
	read(fd, (void*)buf, fd_size - 1);
	close(fd);
    return buf;  
}

int* create_pipes(int count)
{
	int* pipes = (int*)malloc(count * 2 * sizeof(int));
	for (int i = 0; i < count; i++)
	{
		pipe(pipes + i * 2);
	}
	return pipes;
}

void close_pipes (int* pipes, int count)
{
	for (int i = 0; i < count; i++)
		{
			close(pipes[i * 2]); //close input in pipe
			close(pipes[2 * i + 1]); //close output from pipe
		}
}

void do_subcoms(char* command, int* pipes, int count, int id)
{
	int i = 0;
	int length = strlen(command);
	char **arr_subcoms = (char**)malloc(length);
	while ((arr_subcoms[i] = strtok_r(command, " ", &command)))
	{
		i++;
	}

	if (id != 0) //not first input
		dup2(pipes[2 * (id - 1)], STDIN_FILENO);
	if (id < (count - 1)) //not last output
		dup2(pipes[2 * id + 1], STDOUT_FILENO);
	
	close_pipes(pipes, count);
	
	execvp(arr_subcoms[0], arr_subcoms);
}

int get_coms_count(char *all_commands)
{
	int res = 0;
	char* temp = (char*)malloc(sizeof(all_commands));
	strcpy(temp, all_commands);
	char* token;
	while (token = strtok_r(temp, "|", &temp))
		res++;
	return res;
}

void wait_children (int count)
{
	int temp;
	for (int i = 0; i < count; i++)
		wait(&temp);
}

int main(int argc, char** argv)
{
    int fd = my_open (argc, argv);
	
    char* all_commands = my_read(fd);

	int comd_count = get_coms_count(all_commands);

	int* pipes = create_pipes(comd_count);

	char* command;
	int* pointer = NULL;
	pid_t pid = 0;
	for (int i = 0; i < comd_count; i++)
		{
			command = strtok_r(all_commands, "|", &all_commands);
			pid = fork();
			if (pid == 0) //child
				do_subcoms(command, pipes, comd_count, i);
		}
	
	close_pipes(pipes, comd_count);
	wait_children(comd_count);
    return 0;
}