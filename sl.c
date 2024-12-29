/*******************************************************************************
 * Name        : sl.c
 * Author      : Cormac Taylor
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define  READ_END    0
#define  WRITE_END   1
#define  BUFSIZE     4096

void print_func_error(char* func){
    fprintf(stderr, "Error: %s() failed. %s.\n", func, strerror(errno));
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]){

    if(argc != 2){
        fprintf(stderr, "Usage: %s <DIRECTORY>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct dirent* dp;
    struct stat statbuf;

    DIR* open_dir = opendir(argv[1]);
    if(open_dir == NULL){
        if(errno == EACCES || errno == ENOENT){
            fprintf(stderr, "Permission denied. %s cannot be read.", argv[1]);
        } else if (errno == ENOTDIR){
            fprintf(stderr, "The first argument has to be a directory.");
        }
        exit(1);
    }
    if(closedir(open_dir) == -1) print_func_error("closedir");

    int fd_ls_sort[2];
    int fd_sort_parent[2];
    pid_t pid_ls, pid_sort;

    if(pipe(fd_ls_sort) == -1) print_func_error("pipe");
    if(pipe(fd_sort_parent) == -1) print_func_error("pipe");

    if((pid_ls = fork()) == -1) print_func_error("fork");
    if (pid_ls == 0) {

        /* ls child */
        if(close(fd_sort_parent[READ_END]) == -1) print_func_error("close");
        if(close(fd_sort_parent[WRITE_END]) == -1) print_func_error("close");
        if(close(fd_ls_sort[READ_END]) == -1) print_func_error("close");
        if(dup2(fd_ls_sort[WRITE_END], 1) == -1) print_func_error("dup2");
        execlp("ls", "ls", "-1ai", argv[1], NULL);
        fprintf(stderr, "Error: ls failed.\n");
        exit(EXIT_FAILURE);

    } else {

        if((pid_sort = fork()) == -1) print_func_error("fork");
        if(pid_sort == 0){

            /* sort child */
            if(close(fd_ls_sort[WRITE_END]) == -1) print_func_error("close");
            if(dup2(fd_ls_sort[READ_END], 0) == -1) print_func_error("dup2");
            if(close(fd_sort_parent[READ_END]) == -1) print_func_error("close");
            if(dup2(fd_sort_parent[WRITE_END], 1) == -1) print_func_error("dup2");
            if(waitpid(pid_ls, NULL, 0) == -1 && errno != ECHILD) print_func_error("waitpid");
            execlp("sort", "sort", NULL);
            fprintf(stderr, "Error: sort failed.\n");
            exit(EXIT_FAILURE);

        } else {

            /* parent process */
            if(close(fd_ls_sort[READ_END]) == -1) print_func_error("close");
            if(close(fd_ls_sort[WRITE_END]) == -1) print_func_error("close");
            if(close(fd_sort_parent[WRITE_END]) == -1) print_func_error("close");
            if(dup2(fd_sort_parent[READ_END], 0) == -1) print_func_error("dup2");
            if(waitpid(pid_sort, NULL, 0) == -1 && errno != ECHILD) print_func_error("waitpid");

            int num_files = 0;
            char del[] = "\n";
            char* token;
            char buffer[BUFSIZE] = {0};
            int bytesread = read(0, buffer, BUFSIZE);
   
            token = strtok(buffer, del);
   
            while(token != NULL) {
                printf("%s\n", token);
                token = strtok(NULL, del);
                num_files++;
            }

            printf("Total files: %d\n", num_files);
        }    
    }
    return EXIT_SUCCESS;
}