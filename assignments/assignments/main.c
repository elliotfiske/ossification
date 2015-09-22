//
//  main.c
//  assignments
//
//  Created by Elliot Fiske on 9/21/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void doLSProcess();
void doParentProcess();

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
        perror("Fork FAILED");
        exit(1);
    }
    
    if (childPID_ls == 0) { // Child process
        doLSProcess();
    }
    else {                  // Daddy process
        doParentProcess();
    }
    
    
    return 0;
}

/**
 * Exec the LS process and pipe the result to my buddy 'sort' over there
 */
void doLSProcess() {
    close(pipeFileDescriptors[1]);
}

/**
 * Fork the other child (who handles 'sort')
 */
void doParentProcess() {
    childPID_sort = fork();
    if (-1 == childPID_sort) {
        perror("Fork FAILED");
        exit(1);
    }
}
