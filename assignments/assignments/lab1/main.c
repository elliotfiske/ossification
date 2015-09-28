/****
 *  main.c
 *  Lab 1
 *
 *  Created by Elliot Fiske on 9/21/15.
 *  Copyright © 2015 Elliot Fiske. All rights reserved.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

void doLSProcess();
void doSortProcess();
void doParentProcess();
void waitForMyKids();

pid_t childPID_ls;
pid_t childPID_sort;

int pipeFileDescriptors[2];

int main(int argc, const char * argv[]) {
    
    if (-1 == pipe(pipeFileDescriptors)) {
        perror("Pipe FAILED");
        exit(1);
    }
    
    childPID_ls = fork();
    if (-1 == childPID_ls) {
        perror("LS Fork FAILED");
        exit(1);
    }
    
    if (childPID_ls == 0) { /* Child process */
        doLSProcess();
    }
    else {                  /* Daddy process */
        doParentProcess();
    }
    
    return 0;
}

/**
 * Fork the other child (who handles 'sort')
 */
void doParentProcess() {
    childPID_sort = fork();
    if (-1 == childPID_sort) {
        perror("SORT Fork FAILED");
        exit(1);
    }
    
    if (childPID_sort == 0) { /* Child process */
        doSortProcess();
    }
    else {                    /* Daddy process */
        waitForMyKids();
    }
}


/**
 * Exec the 'ls' process and pipe the result to my buddy 'sort' over there
 */
void doLSProcess() {
    close(pipeFileDescriptors[0]);
    
    dup2(pipeFileDescriptors[1], STDOUT_FILENO);
    
    execlp("ls", "ls", (char *)0);
    perror("LS exec FAILED");
    _exit(1);
}


/**
 * Exec the 'sort' process, making sure to pump the output to our 'output' file
 */
void doSortProcess() {
    int outFD;

    close(pipeFileDescriptors[1]);
    
    dup2(pipeFileDescriptors[0], STDIN_FILENO);
    
    outFD = open("outfile", O_WRONLY | O_TRUNC | O_CREAT, 0775);
    if (outFD == -1) {
        perror("outfile");
        exit(1);
    }
    
    dup2(outFD, STDOUT_FILENO);
    
    execlp("sort", "sort", "-r", (char *)0);
    perror("SORT exec FAILED");
    _exit(1);
}

/**
 * Calls wait() a couple times to clean up all my babies when they're dead
 */
void waitForMyKids() {
    int lsResult, sortResult;
    
    /* Make sure our copy of the pipe isn't open so nothing hangs */
    close(pipeFileDescriptors[0]);
    close(pipeFileDescriptors[1]);
    
    waitpid(childPID_ls, &lsResult, 0);
    if (WIFEXITED(lsResult)) {
        if (WEXITSTATUS(lsResult) != 0) {
            exit(1);
        }
    }
    else {
        printf("ls exited abnormally.\n");
        exit(1);
    }
    
    waitpid(childPID_sort, &sortResult, 0);
    if (WIFEXITED(sortResult)) {
        if (WEXITSTATUS(sortResult) != 0) {
            exit(1);
        }
    }
    else {
        printf("sort exited abnormally.\n");
        exit(1);
    }
}
