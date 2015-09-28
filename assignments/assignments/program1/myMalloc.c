//
//  myMalloc.c
//  assignments
//
//  Created by Elliot Fiske on 9/25/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include "myMalloc.h"
#include <unistd.h>
#include <stdlib.h>

#define MEMORY_ALIGNMENT 16
#define CHUNK_SIZE 64*1024

void *doSbrk(int increment);
void initMallocList();
void adjustDataBreak(size_t size);

/** Pointer to the top of the linked list of malloc'd memory */
char *mallocListHead = NULL;

size_t memoryUsedByMalloc;
size_t availableHeapSpace;

int main(int argc, const char * argv[]) {
    
    
}

/**
 * TODO: CHANGE this back to malloc() u goon
 */
void *doMalloc(size_t size) {
    if (!mallocListHead) {
        initMallocList();
    }
    
    adjustDataBreak(size);
}

/**
 * Sets up the very first pointer in our mallocified linked list
 */
void initMallocList() {
    mallocListHead = doSbrk(0);
    if ((int) mallocListHead % MEMORY_ALIGNMENT != 0) {
        mallocListHead = doSbrk((int) mallocListHead % MEMORY_ALIGNMENT);
    }
}

/**
 * Checks to see if we have enough memory in the heap to accomodate
 *  the user's crazy requests.  If we don't, tell sbrk to carve us up
 *  some delicious new memories.
 */
void adjustDataBreak(size_t size) {
    sbrk(CHUNK_SIZE);
}

/**
 * Error-handled sbrk
 */
void *doSbrk(int increment) {
    char *result = sbrk(increment);
    if ((int) result == -1) {
        perror("sbrk");
        exit(1);
    }
    return result;
}

void testLibraries() {
    printf("Hello library world!\n");
}