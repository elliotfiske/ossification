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

tid_t currThreadID = NO_THREAD + 1;
thread currentThread;
thread threadListHead_sched; /* Keep track of all threads for scheduler */
thread threadListTail_sched;

thread threadListHead_lib; /* Keep track of all the threads so we can delete */
                           /*  the threads on lwp_stop() */

void *muhStack;
rfile oldRFile, dummyRFile;
scheduler currScheduler = NULL;

unsigned long *oldStackPointer;

rfile setupArguments(void *arguments) {
    rfile result;
    size_t offset = sizeof(unsigned long);
    
    result.rdi = ((long) arguments);
    result.rsi = ((long) (arguments + offset));
    result.rdx = ((long) (arguments + offset * 2));
    result.rcx = ((long) (arguments + offset * 3));
    result.r8  = ((long) (arguments + offset * 4));
    result.r9  = ((long) (arguments + offset * 5));
    
    return result;
}


/*******    DEFAULT SCHEDULING FUNCTIONS    ********/
thread defaultScheduler_next() {
    thread result = threadListHead_sched;
    thread prevThread = threadListHead_sched;
    
    if (result == NULL) {
        return NULL;
    }
    
    if (result->sched_one == NULL) {
        return result;
    }
    
    result->sched_one = threadListTail_sched;
    
    while (prevThread->sched_one != threadListHead_sched) {
        prevThread = prevThread->sched_one;
    }
    
    threadListHead_sched = prevThread;
    prevThread->sched_one = NULL;
    
    return result;
}

void defaultScheduler_admit(thread newThread) {
    if (threadListHead_sched == NULL) {
        threadListHead_sched = newThread;
        threadListTail_sched = newThread;
    }
    else {
        newThread->sched_one = threadListTail_sched;
        threadListTail_sched = newThread;
    }
}

void defaultScheduler_remove(thread victim) {
    thread prevThread;
    
    if (threadListTail_sched == threadListHead_sched) {
        threadListHead_sched = threadListTail_sched = NULL; /* One left, kill it*/
    }
    else {
        if (victim == threadListTail_sched) {
            threadListTail_sched = victim->sched_one;
        }
        
        prevThread = threadListTail_sched;
        while (prevThread != NULL && prevThread->sched_one != victim) {
            prevThread = prevThread->sched_one;
        }
        
        if (prevThread == NULL) { /* Victim not found in thread list */
            return;
        }
        
        if (victim == threadListHead_sched) {
            threadListHead_sched = prevThread;
        }
        
        prevThread->sched_one = victim->sched_one;
    }
}


/**
 * Spawn a sexy new thread with the specified function to run,
 *  arguments for the function, and requested stack size.
 */
tid_t lwp_create(lwpfun functionToRun, void *arguments, size_t stackSize) {
    fprintf(stderr, "CALLING LWP_CREATE, hello\n");
    thread result = calloc(1, sizeof(context));
    size_t stackBytes = (stackSize + 4) * sizeof(unsigned long);
    
    void *threadStack = malloc(stackBytes);
    uintptr_t alignedStack = (uintptr_t) threadStack;
//    alignedStack += (16 - alignedStack % 16);
    
    void *threadStackBase = (void *) (alignedStack + stackBytes);
    
    result->tid = currThreadID++;
    
    result->stack = threadStack;
    result->stacksize = stackBytes;
    
    result->state = setupArguments(arguments);
    
    threadStackBase -= sizeof(unsigned long);
    threadStackBase -= sizeof(unsigned long);
    threadStackBase -= sizeof(unsigned long);
    threadStackBase -= sizeof(unsigned long);
    
    result->state.rbp = (unsigned long) threadStackBase;
    
    threadStackBase -= sizeof(unsigned long);
    void (*exit_ptr)(void) = &lwp_exit;
    memcpy(threadStackBase, &exit_ptr, sizeof(unsigned long));
    
//    threadStackBase -= sizeof(unsigned long);
//    *((unsigned long*) threadStackBase) = result->state.rbp;
    
    result->state.rbp = (unsigned long) threadStackBase;
    
    threadStackBase -= sizeof(unsigned long);
    memcpy(threadStackBase, &functionToRun, sizeof(unsigned long));
    
    threadStackBase -= sizeof(unsigned long);
    *((unsigned long*) threadStackBase) = result->state.rbp;
    
    result->state.rbp = (unsigned long) threadStackBase;
    
    muhStack = threadStackBase;
    
    /* Build linked list of threads */
    if (threadListHead_lib == NULL) {
        threadListHead_lib = result;
    }
    else {
        result->lib_one = threadListHead_lib;
        threadListHead_lib = result;
    }
    
    if (currScheduler == NULL) {
        defaultScheduler_admit(result);
        
        if (threadListHead_sched == NULL) {
            threadListHead_sched = result;
        }
    }
    
    return result->tid;
}

/**
 * Call this from a lwp thread to tell it to DIE
 */
void lwp_exit(void) {
    thread dummyThread; /* don't  ask */
    
    swap_rfiles(&dummyRFile, &oldRFile);
    
    printf("We're still in lwp_exit boss\n");
//    free(currentThread->stack);
//    if (currScheduler == NULL) {
//        defaultScheduler_remove(currentThread);
//    }
    
//    free(currentThread);
//    oldRFile.rax = 0xDADBEEDF;
    
    return;
}

/**
 * Call this from a thread to get its ID
 */
tid_t lwp_gettid(void) {
    return currentThread->tid;
}

/**
 * Call this from a lwp thread to let the scheduler know we're starting
 *  to block
 */
void  lwp_yield(void) {
    fprintf(stderr, "Called lwp_yield\n");
//    swap_rfiles(&(currentThread->state), &oldRFile);
}

/**
 * Grab the next thread from the scheduler
 */
thread lwp_get_next() {
    if (currScheduler == NULL) {
        return defaultScheduler_next();
    }
    
    return currScheduler->next();
}

/**
 * Start all da threads we've lwp_create'd
 */
void  lwp_start(void) {
    fprintf(stderr, "CALLING LWP_START heyo\n");
    
    thread next = lwp_get_next();
    
    while (next != NULL) {
        currentThread = next;
        swap_rfiles(&oldRFile, &(threadListHead_sched->state));
        next = lwp_get_next();
    }
    
    printf("Made it back to lwp_start, you know\n");
    // free(thread), if thread->tid == 0
    
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
    return currScheduler;
}

/**
 * Given a thread's ID, return that thread (or NO_THREAD) if no
 *  such thread exists.
 */
thread tid2thread(tid_t tid) {
    thread result = threadListHead_lib;
    
    if (threadListHead_lib == NULL) {
        return NO_THREAD;
    }
    
    while (result != NULL && result->tid != tid) {
        result = result->lib_one;
    }
    
    if (result == NULL) {
        return NO_THREAD;
    }
    
    return result;
}

void poop(long a) {
    unsigned long butts = 0xABCDEFAA;
    unsigned long buttz = 0xE69E69EE;
    
    printf("Hi I'm in here now %zu arg: %zu\n", butts, a);
    
    printf("STuff %zu\n", buttz);
    
    printf("HI there you\n");
    
    printf("HI there you\n");
    printf("yielding\n");
//    lwp_yield();
    printf("HI there you\n");
    
    return;
}


int main(int argc, char *argv[]) {
    long argument = 69;
    
    tid_t threadID = lwp_create((lwpfun)poop, (void *)argument, 1000);
//    thread toRun = tid2thread(threadID);
//    defaultScheduler_admit(toRun);
    
    lwp_start();
    printf("I MADE IT BACK!!\n");
    
    return 0;
}