//
//  myMalloc.c
//  assignments
//
//  Created by Elliot Fiske on 9/25/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include "malloc.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define MEMORY_ALIGNMENT 16
#define CHUNK_SIZE 64*1024
#define DEBUG_STR_SIZE 1000

/* How much spare free space we should have if we're going to split
 *  up a block that's been free'd */
#define FREE_MARGIN sizeof(MallocHeader) * 2

void *doSbrk(int increment);
void *initMallocList();
int adjustDataBreak(size_t size);
void *allocateMemoryAt(MallocHeader *currMallocListPosn, size_t size);
void *reuseFreedMemory(MallocHeader *freedMemory, size_t size);
MallocHeader *findOwnerBlock(void *ptr, MallocHeader **prevBlock);
void *expandLastBlock(MallocHeader *lastBlock, size_t size);

/** Pointer to the top of the linked list of malloc'd memory */
MallocHeader *mallocListHead = NULL;

size_t availableMemoryInChunk;

char debugOutputString[DEBUG_STR_SIZE];

int main(int argc, char *argv[]) {
    for (int i = 1; i < 8192; i++) {
        int *test = malloc(1);
        realloc(test, i);
    }
}

void debugMallocOutput(size_t reqSize, size_t actualSize, void *result) {
    if (getenv("DEBUG_MALLOC")) {
        snprintf(debugOutputString,
               DEBUG_STR_SIZE, "MALLOC: malloc(%zu) => (ptr=%p, size=%zu))\n",
                 reqSize, result, actualSize);
        fputs(debugOutputString, stderr);
    }
}

void debugReallocOutput(void *reallocBlock, size_t reqSize, size_t actualSize,
                        void *result) {
    if (getenv("DEBUG_MALLOC")) {
        snprintf(debugOutputString,
        DEBUG_STR_SIZE, "MALLOC: realloc(%p, %zu) => (ptr=%p, size=%zu))\n",
        reallocBlock, reqSize, result, actualSize);
        fputs(debugOutputString, stderr);
    }
}

/**
 * Gives you a chunk of memory on the heap! Guarenteed to be 16-byte
 *  aligned or your money back.
 */
void *malloc(size_t size) {
    MallocHeader *currMallocListPosn = mallocListHead;
    
    if (!adjustDataBreak(size + sizeof(MallocHeader))) {
        errno = ENOMEM; /* Out of memory */
        return NULL;
    }
    
    if (!currMallocListPosn) { /* Very first malloc'd node! */
        currMallocListPosn = mallocListHead;
        
        allocateMemoryAt(currMallocListPosn, size);
        
        debugMallocOutput(size, size, currMallocListPosn->data);
        return currMallocListPosn->data;
    }
    
    if (currMallocListPosn->isFree &&
        currMallocListPosn->dataSize >= size) {
        debugMallocOutput(size, size, currMallocListPosn->data);
        return reuseFreedMemory(currMallocListPosn, size);
    }
    
    while (!currMallocListPosn->isLast) {
        if (currMallocListPosn->isFree &&
            currMallocListPosn->dataSize >= size) {
            debugMallocOutput(size, size, currMallocListPosn->data);
            return reuseFreedMemory(currMallocListPosn, size);
        }
        
        currMallocListPosn = currMallocListPosn->next;
    }
    
    currMallocListPosn->isLast = 0;
    allocateMemoryAt(currMallocListPosn->next, size);
    
    debugMallocOutput(size, size, currMallocListPosn->next->data);
    return currMallocListPosn->next->data;
}

/**
 * Mark the chunk of memory that 'ptr' is in as free
 */
void free(void *ptr) {
    MallocHeader *prevBlock;
    MallocHeader *blockToFree = findOwnerBlock(ptr, &prevBlock);
    
    if ((void*) -1 == blockToFree) {
        return;
    }
    
    blockToFree->isFree = 1;
    
    if (prevBlock && prevBlock->isFree) {
        prevBlock->next = blockToFree->next;
        prevBlock->dataSize += blockToFree->dataSize;
        prevBlock->isLast = blockToFree->isLast;
        
        blockToFree = prevBlock;
    }
    
    if (blockToFree->next && blockToFree->next->isFree) {
        blockToFree->dataSize += blockToFree->next->dataSize;
        blockToFree->isLast = blockToFree->next->isLast;
        
        blockToFree->next = blockToFree->next->next;
    }
}

/**
 * Called malloc(), and found a chunk of free'd memory big enough for us.
 *  Let's use that memory instead of adding another node to the end!
 */
void *reuseFreedMemory(MallocHeader *freedMemory, size_t size) {
    MallocHeader *blockAfterMe = freedMemory->next;
    int wasLastBlock = freedMemory->isLast;
    size_t totalFreeSize = freedMemory->dataSize;
    size_t leftoverSize = totalFreeSize - size;
    MallocHeader *extraFreeBlock;
    
    allocateMemoryAt(freedMemory, size);
        
    /* Check for extra unused space we can fit another 'free' block into */
    if (leftoverSize > FREE_MARGIN) {
        allocateMemoryAt(freedMemory->next,
                         leftoverSize - FREE_MARGIN);
        extraFreeBlock = freedMemory->next;
        extraFreeBlock->isFree = 1;
        extraFreeBlock->next = blockAfterMe;
        extraFreeBlock->isLast = wasLastBlock;
        
        freedMemory->isLast = 0;
    }
    else {
        freedMemory->next = blockAfterMe;
        freedMemory->isLast = wasLastBlock;
    }
    
    return freedMemory->data;
}

/**
 * Re-allocate a chunk of memory allready malloc'd, to a different size
 */
void *realloc(void *ptr, size_t size) {
    MallocHeader *blockToRealloc;
    MallocHeader *prevBlock;
    size_t joinedFreeBlockMemory;
    
    void *result;
    
    if (!ptr) {
        result = malloc(size);
        debugReallocOutput(ptr, size, size, result);
        return result;
    }
    
    if (size == 0) {
        free(ptr);
        debugReallocOutput(ptr, size, size, 0);
        return NULL;
    }
    
    blockToRealloc = findOwnerBlock(ptr, &prevBlock);

    /* Check if there's a free block right after us that we can co-opt */
    if (blockToRealloc->next && !blockToRealloc->isLast) {
        joinedFreeBlockMemory = blockToRealloc->dataSize +
                                blockToRealloc->next->dataSize;
        
        if (blockToRealloc->next && blockToRealloc->next->isFree &&
            joinedFreeBlockMemory > size) {
            blockToRealloc->isFree = 1;
            
            blockToRealloc->dataSize += blockToRealloc->next->dataSize;
            blockToRealloc->isLast = blockToRealloc->next->isLast;
            
            blockToRealloc->next = blockToRealloc->next->next;
        }
    }
    
    if (blockToRealloc->dataSize >= size) { /* Can shrink block in-place */
        blockToRealloc->isFree = 1;
        debugReallocOutput(ptr, size, size, blockToRealloc->data);
        return reuseFreedMemory(blockToRealloc, size);
    }
    else {
        /* Reallocing the last block in the list. Just expand it. */
        if (blockToRealloc->isLast) {
            debugReallocOutput(ptr, size, size, blockToRealloc->data);
            return expandLastBlock(blockToRealloc, size);
        }
    }
    
    /* Couldn't do anything clever. Set old block as free and give 'em
     *  a new block, with the same data. */
    result = malloc(size);
    if (result) {
        memcpy(result, blockToRealloc->data, size);
        free(blockToRealloc->data);
    }
    debugReallocOutput(ptr, size, size, result);
    return result;
}

/**
 * Given a pointer, return the malloc'd block that contains it.
 *  Also sets *prevBlock to point to the block right before it.
 *  If none found, return -1.
 */
MallocHeader *findOwnerBlock(void *ptr, MallocHeader **prevBlock) {
    MallocHeader *currMallocListPosn = mallocListHead;
    MallocHeader *prevMallocListPosn = NULL;
    
    uintptr_t pointerValue = (uintptr_t) ptr;
    uintptr_t currDataEndValue;
    
    if (!ptr || !currMallocListPosn) { /* Either we're trying to find NULL, */
        return (void*) -1;             /* or we tried free() or realloc()   */
    }                                  /*  before malloc() was ever called. */
    
    if (pointerValue <= (uintptr_t) mallocListHead) {
        return (void*) -1;
    }
    
    while (!currMallocListPosn->isLast) {
        currDataEndValue = (uintptr_t) currMallocListPosn->data +
                           (uintptr_t) currMallocListPosn->dataSize;
        if (currDataEndValue >= pointerValue) {
            break;
        }
        
        prevMallocListPosn = currMallocListPosn;
        currMallocListPosn = currMallocListPosn->next;
    }
    
    if (currMallocListPosn->next == NULL) {
        uintptr_t endOfHeap = (uintptr_t)currMallocListPosn->data +
                              (uintptr_t)currMallocListPosn->dataSize;
        if (pointerValue > endOfHeap) {
            return (void*) -1;
        }
    }
    
    *prevBlock = prevMallocListPosn;
    return currMallocListPosn;
}

/**
 * We've found where we want to malloc memory!
 *  Set up the MallocHeader there and return where the data will go.
 */
void *allocateMemoryAt(MallocHeader *currMallocListPosn, size_t size) {
    uintptr_t newDataLocation = (uintptr_t) currMallocListPosn
                                            + sizeof(MallocHeader);

    newDataLocation += (MEMORY_ALIGNMENT - newDataLocation % MEMORY_ALIGNMENT);
    
    uintptr_t nextHeaderLocation = newDataLocation + size;
    
    MallocHeader newMallocHeader;
    newMallocHeader.isFree = 0;
    newMallocHeader.isLast = 1;
    newMallocHeader.dataSize = size;
    newMallocHeader.next = (MallocHeader *) nextHeaderLocation;
    newMallocHeader.data = (void *) newDataLocation;
    
    memcpy(currMallocListPosn, &newMallocHeader, sizeof(MallocHeader));
    
    return newMallocHeader.data;
}

void *calloc(size_t nmemb, size_t size) {
    size_t totalSize = nmemb * size;
    void *result = malloc(totalSize);
    
    if (result) {
        memset(result, 0, totalSize);
    }
    
    return result;
}

/**
 * Checks to see if we have enough memory in the heap to accomodate
 *  the user's requests.  If we don't, tell sbrk to carve us up
 *  some delicious new memory.  Returns FALSE if there's no memory
 *  left, so malloc() can return NULL.
 */
int adjustDataBreak(size_t size) {
    int numChunks = 0;
    
    while (availableMemoryInChunk + numChunks * CHUNK_SIZE < size) {
        numChunks++;
    }
    
    if (numChunks > 0) {
        void *result = sbrk(numChunks * CHUNK_SIZE);
        if ((void*) -1 == result) {
            perror("sbrk");
            return 0;
        }
        
        if (mallocListHead == NULL) {
            mallocListHead = result;
        }
    }
    
    availableMemoryInChunk = (availableMemoryInChunk + size) % CHUNK_SIZE;
    return 1;
}

/**
 * Called realloc() on the last block.  Expand it in-place.
 */
void *expandLastBlock(MallocHeader *lastBlock, size_t extraSize) {
    if (!adjustDataBreak(extraSize)) {
        errno = ENOMEM; /* Out of memory */
        return NULL;
    }
    
    allocateMemoryAt(lastBlock, lastBlock->dataSize + extraSize);
    return lastBlock->data;
}