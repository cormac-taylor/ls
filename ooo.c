/*******************************************************************************
 * Name        : sl.c
 * Author      : Vraj Patel
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

struct ln {
    char* pid;
    char* f;
};

int main(int argc, char const *argv[])
{
    int fd[2];
    int fd2[2];
    if(pipe(fd) == -1){
        fprintf(stderr, "Error: pipe failed");
        exit(EXIT_FAILURE);
    }
    if(pipe(fd2) == -1){
        fprintf(stderr, "Error: pipe failed");
        exit(EXIT_FAILURE);
    }

    if(argc != 2){
        fprintf(stderr, "The correct usage is ./sl <directory>\n");
        exit(EXIT_FAILURE);
    }

    DIR* dp;
    struct stat fileinfo;
    int status;

    dp = opendir(argv[1]);
    status = stat(argv[1], &fileinfo);
    if (status == -1 || ((S_ISDIR(fileinfo.st_mode) == 1) && dp == NULL)){
        fprintf(stderr, "Permission denied. %s cannot be read.", argv[1]);
        exit(EXIT_FAILURE);
    }
    else{
        if(dp == NULL) {
            fprintf(stderr, "The first argument has to be a directory.");
            exit(EXIT_FAILURE);
        }
    }
    

    // ls child
    int stat;
    pid_t p1 = fork();
    if(p1 < 0){
        fprintf(stderr, "Error: fork() failed.\n");
        exit(EXIT_FAILURE);
    }
    else if(p1 == 0){
        close(fd[0]);
        dup2(fd[1], 1); 
        close(fd[1]);
        char* args[] = {"ls", "-1ai", argv[1], 0};
        int ex = execvp("ls", args);
        if(ex < 0){
            fprintf(stderr, "Error: ls failed.\n");
            exit(EXIT_FAILURE);
        }
        
    }
    else{

        // sort child
        int p2 = fork();
        if(p2 < 0){
            fprintf(stderr, "Error: fork() failed.\n");
            exit(EXIT_FAILURE);
        }
        else if(p2 == 0){
            close(fd[1]);
            dup2(fd[0], 0); 
            close(fd[0]);

            close(fd2[0]);
            dup2(fd2[1], 1);
            close(fd2[1]);

            char* args[] = {"sort", NULL};
            int ex = execvp("sort", args);
            if(ex < 0){
                fprintf(stderr, "Error: sort failed.\n");
            }
        }
        else{
            close(fd[0]);
            close(fd[1]);
            int x = waitpid(p1, &stat, 0);

            close(fd2[1]);
            dup2(fd2[0], 0); 
            close(fd2[0]);

            int total_files = 0;
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                printf("%s", buffer);
                total_files++;
            }

            close(fd2[0]);
            close(fd2[1]);
            int y = waitpid(p2, &stat, 0);
            printf("Total files: %d\n", total_files);
        }

    }

    return 0;
}
