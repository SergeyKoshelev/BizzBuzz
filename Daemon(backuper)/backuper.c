#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define BUF_SIZE 1024 //size for copying buf
#define PATH_SIZE 128 //max path
#define lOG_SIZE 512 //max size of log message
#define OLD 0  //old directory
#define NEW 1  //maybe new directory
#define EVENTS_SIZE 64 //size of events array

const int mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_TO | IN_MOVED_FROM; //mask for inotify
const char my_fifo[] = "fifo_log";

#define SIG_STOP SIGINT
#define SIG_CHECK SIGUSR2

int in = -1; //fd of inotify
int in_flag = 1; //flag of working process and inotify
int check_flag = 1; //1)flag of moving to new directory (not to go in for if new process created)
                    //2)just stop checking events for self_delete and seld_move

char log_str[lOG_SIZE];
char src_path[PATH_SIZE];
char dest_path[PATH_SIZE];
DIR* source;
DIR* dest;
int log_fd = -1;

pid_t main_pid;

//send signal to all children
int send_children(int signal)
{
    pid_t my_pid = getpid();
    setgid(my_pid + 1);
    killpg(my_pid, signal);
}

//log smth
void daemon_log()
{
    //printf("%s", log_str);
    int count = write(log_fd, log_str, strlen(log_str));
    //printf("count: %d\n", count);
    //char buf[1024];
    //read(log_fd, buf, 1024);
    //printf("buf: (%s)", buf);
}

//start log (create fifo and connect to it)
void start_log()
{
    log_fd = open(my_fifo, O_WRONLY);
    if (log_fd == -1) 
    {
        printf("Can't open fifo %s for logging\n", my_fifo);
        exit(1);
    }
}

//check if file is regular
int is_regular(char* path)
{
    struct stat stat;
    lstat(path, &stat);
	if (((stat.st_mode) & S_IFMT) != S_IFREG)
        return 0;
    return 1;
}

//check if file is directory
int is_directory(char* path)
{
    struct stat stat;
    lstat(path, &stat);
	if (((stat.st_mode) & S_IFMT) != S_IFDIR)
        return 0;
    return 1;
}

//opening directory related to path; flag = OLD for existing and flag = NEW for probably new 
DIR* open_dir(char* path, int flag)
{
    DIR* dir = NULL;
    dir = opendir(path);
    if ((flag == NEW) && (dir == NULL))
    {
        int check = mkdir(path, S_IRWXU);
        if (check != 0)
        {
            sprintf(log_str, "Can't create directory %s\tPid: %d\n", path, getpid());
            daemon_log();
            exit(1);
        }
        dir = opendir(path);
    }
    if (dir == NULL)
    {
        sprintf(log_str, "Can't open directory %s\tPid: %d\n", path, getpid());
        daemon_log();
        exit(1);
    }
    return dir;
}

//make path file_path = dir_path/name
int make_path(char* file_path, char* dir_path, char* name)
{
    if (strcmp(file_path, dir_path))
    {
        file_path[0] = '\0';
        strcpy(file_path, dir_path);
    }
    strcat(file_path, "/");
    strcat(file_path, name);
}

//copy file (ONLY COMMON) from path source to path dest
int copy_file (char* source, char* dest)
{
    sprintf(log_str, "Copy file: %s to %s\n", source, dest);
    daemon_log();

    pid_t pid = fork();
    if (pid == 0)
    {
        int res = execlp("cp", "cp", source, dest, NULL);
        printf("Error in exec in copy\n");
        exit(1);
    }
    waitpid(pid, NULL, 0);
}

//1)init inotify and write its fd in "in"
//2) add watch on directory
int create_inotify()
{
    in = inotify_init();
    if (in < 0)
    {
        sprintf(log_str, "Can't create iniotify\n");
        daemon_log();
        exit(1);
    }

    int watch = inotify_add_watch(in, src_path, mask);
    if (watch < 0)
    {
        sprintf(log_str, "Can't add watch\n");
        daemon_log();
        exit(1);
    }
}

//first copying process's directory (call only in new process!)
//1)making exactly one process for each directory
//2)change src_path and dest_path, open them with open_dir
int copy_directory()
{
    create_inotify();
    source = open_dir(src_path, OLD);
    dest = open_dir(dest_path, NEW);
    char file_path_src[PATH_SIZE];
    char file_path_dest[PATH_SIZE];
    struct dirent* file;
    pid_t pid = getpid();
    while ((file = readdir(source)) && (pid != 0))
    {
        if (strcmp(file->d_name, ".") && strcmp(file->d_name, ".."))
        {
            make_path(file_path_src, src_path, file->d_name);

            if (file->d_type & DT_REG)
            {           
                copy_file(file_path_src, dest_path);
            }
            else if (file->d_type & DT_DIR)
            {
                pid = fork();
                if (pid == 0)
                {
                    make_path(src_path, src_path, file->d_name);
                    make_path(dest_path, dest_path, file->d_name);
                    sprintf(log_str, "New process!\t Pid: %d\t Directory: %s\t\n", getpid(), src_path);
                    daemon_log();
                    copy_directory();
                }
            }
            else
            {
                sprintf(log_str, "Dont work with such files\n");
                daemon_log();
            }
        }
    }
}

//delete file or directory
int del(char* path)
{
    sprintf(log_str, "Delete: %s\n", path);
    daemon_log();

    pid_t pid = fork();
    if (pid == 0)
    {
        int res = execlp("rm", "rm", "-r", path, NULL);
        sprintf(log_str, "Error in exec in delete\n");
        daemon_log();
        exit(1);
    }
    waitpid(pid, NULL, 0);
    send_children(SIG_CHECK);
}

//under construction
//check if folder is alife
int is_alife(char* path)
{
    struct stat stat;
    int res = lstat(path, &stat);
	if (res == -1)
         return 0;
    else
        return 1;
}

//finish woriking with directory and kill children
void finish()
{
    if (is_alife(src_path))
        closedir(source);
    if (is_alife(dest_path))
        closedir(dest);
    close(in);
    send_children(SIG_STOP);
    sprintf(log_str, "End of watching!\t Pid: %d\t Directory: %s\n", getpid(), src_path);
    daemon_log();

    if (getpid() == main_pid)
    {
        sleep(1);
        sprintf(log_str, "END OF BACKUP!\tFolder:%s\n\n", src_path);
        daemon_log();
    }
    exit(0);
}

//one of the folders was deleted in parent, check if it is current
void check_folder()
{
    if (!is_alife(src_path))
        finish();
}

//file was created
int event_create(struct inotify_event* event)
{
    char file_path_src[PATH_SIZE];
    char file_path_dest[PATH_SIZE];
    make_path(file_path_src, src_path, event->name);
    make_path(file_path_dest, dest_path, event->name);
    
    if (is_regular(file_path_src))
        copy_file(file_path_src, dest_path);
    else if (is_directory(file_path_src))
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            strcpy(src_path, file_path_src);
            strcpy(dest_path, file_path_dest);
            sprintf(log_str, "New process!\t Pid: %d\t Directory: %s\t(in event_create)\n", getpid(), src_path);
            daemon_log();
            copy_directory();
            check_flag = 0;
        }
    }
}

//file was deleted
int event_delete(struct inotify_event* event)
{
    char path[PATH_SIZE];
    make_path(path, dest_path, event->name);
    del(path);
}

//file was modified (=create)
int event_modify(struct inotify_event* event)
{
    event_create(event);
}

//file moved to directory (=create)
int event_move_to(struct inotify_event* event)
{
    usleep(1000);  //add sleep for moving directory, because files can't appear so fast
    event_create(event);
}

//file deleted from directory (=delete)
int event_move_from(struct inotify_event* event)
{
    event_delete(event);
}

//check event 
int check_event(struct inotify_event* events, int i)
{
    struct inotify_event* event = events + i;
    if ((event->mask & IN_CREATE) && (event->len))
    {
        sprintf(log_str, "%d)event: create\t file: %s/%s\n", i + 1, src_path, event->name);
        daemon_log();
        event_create(event);
    }
    else if ((event->mask & IN_DELETE) && (event->len))
    {
        sprintf(log_str, "%d)event: delete\t file: %s/%s\n", i + 1, src_path, event->name);
        daemon_log();
        event_delete(event);
    }
    else if ((event->mask & IN_MODIFY) && (event->len))
    {
        sprintf(log_str, "%d)event: modify\t file: %s/%s\n", i + 1, src_path, event->name);
        daemon_log();
        event_modify(event);
    }
    else if ((event->mask & IN_MOVED_TO) && (event->len))
    {
        sprintf(log_str, "%d)event: move_to\t file: %s/%s\n", i + 1, src_path, event->name);
        daemon_log();
        event_move_to(event);
    }
    else if ((event->mask & IN_MOVED_FROM) && (event->len))
    {
        sprintf(log_str, "%d)event: move_from\t file: %s/%s\n", i + 1, src_path, event->name);
        daemon_log();
        event_move_from(event);
    }
}

//start daemon
int start_daemon()
{
    pid_t tmp_pid = fork();
    if (tmp_pid != 0)
    {
        printf("Start daemon with pid: %d\t Source: %s\t Destination: %s\n", tmp_pid, src_path, dest_path);
        sleep(2);
        exit(0);
    }
    start_log(); //start log in daemon
    sprintf(log_str, "\nStart daemon with pid: %d\t Source: %s\t Destination: %s\n", getpid(), src_path, dest_path);
    daemon_log();
}

int main(int argc, char** argv)
{
    assert((argc == 3) && "Incorrect count of arguments");
    strcpy(src_path, argv[1]);
    strcpy(dest_path, argv[2]);

    start_daemon();
    main_pid = getpid();
    
    signal(SIG_STOP, finish);
    signal(SIG_CHECK, check_folder);

    copy_directory();
    struct inotify_event events[EVENTS_SIZE]; 
    int count = 0;
    int i = 0;
    while (in_flag)
    {
        check_flag = 1;
        int res_read = read(in, (void*)events, sizeof(events));
        count = res_read / sizeof(struct inotify_event);
        if (count <= 0)  //check if directory exitst
            in_flag = 0;
        for (i = 0; (i < count) && check_flag; i++)
        {
            check_folder();
            check_event(events, i);
        }
    }

    finish();
}
