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
    char* buf = (char*)malloc(fd_size * sizeof(char));
	read(fd, (void*)buf, fd_size - 1);
	close(fd);
    return buf;  
}

int* create_pipes(int count)
{
	//printf("maloc in create pipes start\n");
	int* pipes = (int*)malloc(count * 2 * sizeof(int));
	//printf("maloc in create pipes done\n");
	for (int i = 0; i < count; i++)
	{
		pipe(pipes + i * 2);
		//printf("create pipe num: %d\n", i);
	}
	return pipes;
}

void close_pipes (int* pipes, int count)
{
	for (int i = 0; i < count; i++)
		{
			close(pipes[i * 2]); //close input in pipe
			close(pipes[2 * i + 1]); //close output from pipe
			//printf("close pipe num: %d\n", i);
		}
}

void do_subcoms(char* command, int* pipes, int count, int id)
{
	//printf ("pipe_in: %d   pipe_out: %d\n", pipe_in, pipe_out);	
	int i = 0;
	int length = strlen(command);
	//printf("malloc in do subcoms\n");
	char **arr_subcoms = (char**)malloc(count * sizeof(char*));
	while ((arr_subcoms[i] = strtok_r(command, " ", &command)))
	{
		printf("token: (%s)\n", arr_subcoms[i]);
		i++;
	}
	
	//printf ("count of subcoms: %d\n", count);
	//printf ("pipe_in: %d   pipe_out: %d\n", pipe_in, pipe_out);	

	if (id != 0) //not first input
		dup2(pipes[2 * (id - 1)], STDIN_FILENO);
	if (id < (count - 1)) //not last output
		dup2(pipes[2 * id + 1], STDOUT_FILENO);
	
	close_pipes(pipes, count);

	//printf("arr[0]: %s\n", arr_subcoms[0]);
	
	execvp(arr_subcoms[0], arr_subcoms);
}
/*
char* get_data_from_pipe(int pipe_fd)
{
	
	//struct stat* pipe_stat = malloc (sizeof(struct stat));
	//fstat(pipe_fd, pipe_stat);
	//off_t size_pipe = pipe_stat->st_size;
	//printf("size of pipe: %ld\n", size_pipe);
	

    char* buf = (char*)malloc(size_pipe);
	read(pipe_fd, (void*)buf, size_pipe);

	char* buf = (char*)malloc(100);
	int check = 0;
	read(pipe_fd, (void*)buf, 10000000000000);

	
	//for (size_t i = 0; i < 10000000000000; i++)
	//{
	//	scanf("%s", buf);
	//	printf ("data: %s\n", buf);	
	//}
	

	free(pipe_stat);
	return buf;
}
*/
int get_coms_count(char *all_commands)
{
	int res = 0;
	//printf("malloc in get coms count\n");
	char* temp = (char*)malloc(sizeof(all_commands));
	strcpy(temp, all_commands);
	char* token;
	while (token = strtok_r(temp, "|", &temp))
		res++;
	//printf("im here\n");
	return res;
}

void wait_children (int count)
{
	int temp;
	for (int i = 0; i < count; i++)
		wait(&temp);
	//free(temp);
}

int main(int argc, char** argv)
{
    int fd = my_open (argc, argv);
	
    char* all_commands = my_read(fd);

	int comd_count = get_coms_count(all_commands);
	printf("commands: (%s)\t count: %d\n", all_commands, comd_count);
		
	//int data_pipe[2] = {};
	//pipe(data_pipe);
	
	int* pipes = create_pipes(comd_count);
	
	//printf ("pipe_in: %d   pipe_out: %d\n", data_pipe[0], data_pipe[1]);

	char* command;
	int* pointer = NULL;
	pid_t pid = 0;
	for (int i = 0; i < comd_count; i++)
		{
			command = strtok_r(all_commands, "|", &all_commands);
			//printf ("command: %s\n", command);
			pid = fork();
			if (pid == 0) //child
				do_subcoms(command, pipes, comd_count, i);
		}
	
	//dup2(data_pipe[0], STDIN_FILENO);
	//char* res = get_data_from_pipe(data_pipe[0]);
	
	close_pipes(pipes, comd_count);
	wait_children(comd_count);
	
	//printf("res: %s\n", res);
	
	//free(pipes);
	///free(all_commands);

    return 0;
}