//
//  main.c
//  program2
//
//  Created by Elliot Fiske on 10/6/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include <stdio.h>
#include "lwp.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

tid_t currThreadID = 0;
thread threadListHead; /* Keep track of all the threads so we can delete
                           the threads on lwp_stop() */

void *muhStack;

rfile setupArguments(void *arguments) {
    rfile result;
    size_t offset = sizeof(unsigned long);
    
    result.rdi = *((int *) arguments);
    result.rsi = *((int *) (arguments + offset));
    result.rdx = *((int *) (arguments + offset));
    result.rcx = *((int *) (arguments + offset));
    result.r8  = *((int *) (arguments + offset));
    result.r9  = *((int *) (arguments + offset));
    
    return result;
}

/**
 * Spawn a sexy new thread with the specified function to run,
 *  arguments for the function, and requested stack size.
 */
tid_t lwp_create(lwpfun functionToRun, void *arguments, size_t stackSize) {
    thread result = malloc(sizeof(context));
    size_t stackBytes = stackSize * sizeof(unsigned long);
    
    void *threadStack = malloc(stackBytes);
    uintptr_t alignedStack = (uintptr_t) threadStack;
    alignedStack += (16 - alignedStack % 16);
    threadStack = (void *)alignedStack;
    
    void *threadStackBase = threadStack + stackBytes;
    
    result->tid = currThreadID++;
    
    result->stack = threadStack;
    result->stacksize = stackBytes;
    
    result->state = setupArguments(arguments);
    
    // TODO: put something here: old base pointer?
    threadStackBase -= sizeof(unsigned long);
    
    threadStackBase -= sizeof(unsigned long);
    void (*exit_ptr)(void) = &lwp_exit;
    memcpy(threadStackBase, &exit_ptr, sizeof(unsigned long));
    
    
//    result->state.rbp = (unsigned long) threadStackBase; TODO: wuts going on here
    threadStackBase -= sizeof(unsigned long);
    memcpy(threadStackBase, &functionToRun, sizeof(unsigned long));
    
    threadStackBase -= sizeof(unsigned long);
    *((unsigned long*) threadStackBase) = 0xABCD1234;
    
    result->state.rbp = (unsigned long) threadStackBase;
    
    /* Build linked list of threads */
    if (threadListHead == NULL) {
        threadListHead = result;
    }
    else {
        threadListHead->lib_one = result;
        result->lib_two = threadListHead;
    }
    
    muhStack = threadStackBase;
    
    threadListHead = result;
    
    return result->tid;
}

/**
 * Call this from a lwp thread to tell it to DIE
 */
void  lwp_exit(void) {
    int a = 10;
    fprintf(stderr, "Called lwp_exit %d\n", a);
    
    return;
}

/**
 * Call this from a thread to get its ID
 */
tid_t lwp_gettid(void) {
    tid_t result;
    fprintf(stderr, "Called lwp_gettid\n");
    return result;
}

/**
 * Call this from a lwp thread to let the scheduler know we're starting
 *  to block
 */
void  lwp_yield(void) {
    fprintf(stderr, "Called lwp_yield\n");
}

/**
 * Start all da threads we've lwp_create'd
 */
void  lwp_start(void) {
    rfile oldRfiles;
    
    swap_rfiles(&oldRfiles, &(threadListHead->state));
}

/**
 * Stop all threads!
 */
void  lwp_stop(void) {
    fprintf(stderr, "Called lwp_stop\n");
}

/**
 * Give our lwp system a new scheduler. Rips out all the
 *  old threads from the old scheduler (if any) and puts
 *  them in the new scheduler, then calls init() on the
 *  new scheduler.
 */
void  lwp_set_scheduler(scheduler fun) {
    fprintf(stderr, "Called lwp_set_scheduler\n");
}

/**
 * Returns the scheduler struct that is currently deciding
 *  the fate of our threads.
 */
scheduler lwp_get_scheduler(void) {
    scheduler result;
    fprintf(stderr, "Called lwp_get_scheduler\n");
    return result;
}

/**
 * Given a thread's ID, return that thread (or NULL) if no
 *  such thread exists.
 */
thread tid2thread(tid_t tid) {
    fprintf(stderr, "Called tid2thread\n\n");
    thread result;
    return result;
}

void poop(int a) {
    unsigned long butts = 0xABCDEFAA;
//    unsigned long buttz = 0xE69E69EE;
    
    printf("Hi I'm in here now %zu\n", butts);
    
//    printf("STuff %zu\n", buttz);
    
    return;
}


int main(int argc, char *argv[]) {
    int argument = 69;
    
    lwp_create((lwpfun)poop, &argument, 100);
    
    lwp_start();
    
    return 0;
}

