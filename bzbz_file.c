#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printf("Incorrect count of arguments\n");
		exit(0);
	}
	int fd_src = open(argv[1], O_RDONLY);
	if (fd_src < 0)
	{
		printf("Error with reading\n");
		exit(1);
	}

	struct stat* fd_stat = malloc(sizeof(struct stat));
	fstat(fd_src, fd_stat);
	off_t size_src = fd_stat->st_size;
	if (((fd_stat->st_mode) & S_IFMT) != S_IFREG)
	{
		printf("Not a regular file\n");
		free(fd_stat);
		close(fd_src);
		exit(5);
	}
	int fd_dir = open(argv[2], O_RDWR);
	if (fd_dir >= 0)
		unlink(argv[2]);
	fd_dir = open(argv[2], O_CREAT | O_RDWR, 0666);


	void* temp = malloc(1);
	int last_is_5 = 0;
	int sum = -3;
	int number = 0;
	off_t prev_off = 0;
	int check_is_num = 1;
	ssize_t read_size = 0;
	int negative = 0;
	char prev_symbol = '1';
	char prev_sep = '0';

	for (off_t i = 0; i < size_src; i++)
	{
		prev_symbol = *(char*)temp;
		read_size = read(fd_src, temp, 1);
		if (read_size != 1)
		{
			printf("Error with reading symbol\n");
			exit(3);
		}

		if ((*(char*)temp == ' ') || (i == (size_src - 1)) || (*(char*)temp == '\n') || (*(char*)temp == '\t'))
		{
			if (last_is_5 && (sum == 0) && check_is_num)
			{
				if (prev_sep != '0')
					write(fd_dir, &prev_sep, 1);
				write(fd_dir, &"bizzbuzz", 8);
			}
			else if (last_is_5 && check_is_num)
			{
				if (prev_sep != '0')
					write(fd_dir, &prev_sep, 1);
				write(fd_dir, &"buzz", 4);
			}
			else if ((sum == 0) && check_is_num)
			{
				if (prev_sep != '0')
					write(fd_dir, &prev_sep, 1);
				write(fd_dir, &"bizz", 4);
			}
			else
			{
				lseek(fd_src, prev_off, SEEK_SET);
				if (prev_sep != '0')
					write(fd_dir, &prev_sep, 1);

				if (negative)
					write(fd_dir, &"-", 1);
				for (off_t j = 0; j < i - prev_off; j++)
				{
					read(fd_src, temp, 1);
					write(fd_dir, temp, 1);
				}
				read(fd_src, temp, 1);
			}
			prev_off = i + 1;
			last_is_5 = 0;
			sum = -3;
			check_is_num = 1;
			negative = 0;
			prev_sep = *(char*)temp;

		}
		else
		{
			number = *(char*)temp - 48;
			if (*(char*)temp == '-')
			{
				if ((prev_symbol == ' ') || (i == 0) || (prev_symbol == '\n') || (prev_symbol == '\t'))
				{
					negative = 1;
					prev_off++;
				}
				else
					check_is_num = 0;
			}
			else if ((number < 0) || (number > 9))
				check_is_num = 0;
			else
			{
				if (number % 5 == 0)
					last_is_5 = 1;
				else
					last_is_5 = 0;
				sum += number;
				sum %= 3;
			}
		}
	}

	write(fd_dir, &"\n", 1);
	close(fd_src);
	close(fd_dir);
	free(temp);
}
