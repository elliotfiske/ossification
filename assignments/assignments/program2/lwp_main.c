//
//  lwp_main.c
//  program2
//
//  Created by Elliot Fiske and Jack Wang on 10/6/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include <stdio.h>
#include "lwp.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

tid_t currThreadID = NO_THREAD + 1;
thread currentThread = NULL;
thread threadListTail_sched = NULL; /*Keep track of all threads for scheduler*/
thread threadListHead_sched = NULL;

thread threadListHead_lib = NULL; /* Keep track of all the threads so we can */
                                  /* delete the threads on lwp_stop() */

rfile oldRFile, dummyRFile;
scheduler currScheduler = NULL;

unsigned long *oldStackPointer;

int stopFlag = 0;
int yieldFlag = 0;

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
    
    if (result == NULL) {
        return NULL;
    }
    
    if (result->sched_one == NULL) {
        return result;
    }
    
    /* Add HEAD to the end of the list, after TAIL */
    threadListTail_sched->sched_one = threadListHead_sched;
    /* Set the new HEAD to the guy that was right after HEAD */
    threadListHead_sched = threadListHead_sched->sched_one;
    
    /* The old HEAD now has nothing after him, and has become the tail. */
    result->sched_one = NULL;
    threadListTail_sched = result;
    
    return result;
}

void defaultScheduler_admit(thread newThread) {
    if (threadListTail_sched == NULL) {
        threadListTail_sched = newThread;
        threadListHead_sched = newThread;
    }
    else {
        threadListTail_sched->sched_one = newThread;
        threadListTail_sched = newThread;
    }
}

void defaultScheduler_remove(thread victim) {
    thread prevThread;
    
    if (threadListHead_sched == threadListTail_sched) { /* One left, kill it */
        threadListTail_sched = threadListHead_sched = NULL;
    }
    else {
        if (victim == threadListHead_sched) {
            threadListHead_sched = victim->sched_one;
        }
        
        prevThread = threadListHead_sched;
        while (prevThread != NULL && prevThread->sched_one != victim) {
            prevThread = prevThread->sched_one;
        }
        
        if (prevThread == NULL) { /* Victim not found in thread list */
            return;
        }
        
        if (victim == threadListTail_sched) {
            threadListTail_sched = prevThread;
        }
        
        prevThread->sched_one = victim->sched_one;
    }
}

void libraryList_remove(thread victim) {
    thread prevThread;
    
    if (threadListHead_lib == NULL) {
        return;
    }
    
    if (victim == threadListHead_lib) {
        threadListHead_lib = threadListHead_lib->lib_one;
        return;
    }
    
    if (threadListHead_lib->lib_one == NULL) { /* One thread left */
        if (threadListHead_lib != victim) {
            return;
        }
        threadListHead_lib = NULL;
        return;
    }
    
    prevThread = threadListHead_lib;
    while (prevThread != NULL && prevThread->lib_one != victim) {
        prevThread = prevThread->lib_one;
    }
    
    if (prevThread == NULL) { /* Victim not in list */
        return;
    }
    
    prevThread->lib_one = victim->lib_one;
}


/**
 * Spawn a sweet new thread with the specified function to run,
 *  arguments for the function, and requested stack size.
 */
tid_t lwp_create(lwpfun functionToRun, void *arguments, size_t stackSize) {
    thread result = calloc(1, sizeof(context));
    size_t stackBytes = (stackSize + 4) * sizeof(unsigned long);
    
    void *threadStack = calloc(1, stackBytes);
    uintptr_t alignedStack = (uintptr_t) threadStack;
    
    if (result == NULL || threadStack == NULL) {
        return -1;
    }
    
    void *threadStackBase = (void *) (alignedStack + stackBytes);
    
    result->tid = ++currThreadID;
    
    result->stack = threadStack;
    result->stacksize = stackBytes;
    
    result->state = setupArguments(arguments);
    
    threadStackBase -= sizeof(unsigned long); /* Stack breathing room */
    threadStackBase -= sizeof(unsigned long);
    threadStackBase -= sizeof(unsigned long);
    threadStackBase -= sizeof(unsigned long);
    
    result->state.rbp = (unsigned long) threadStackBase;
    
    threadStackBase -= sizeof(unsigned long);
    void (*exit_ptr)(void) = &lwp_exit;
    memcpy(threadStackBase, &exit_ptr, sizeof(unsigned long));
    
    result->state.rbp = (unsigned long) threadStackBase;
    threadStackBase -= sizeof(unsigned long);
    memcpy(threadStackBase, &functionToRun, sizeof(unsigned long));
    
    threadStackBase -= sizeof(unsigned long);
    *((unsigned long*) threadStackBase) = result->state.rbp;
    
    result->state.rbp = (unsigned long) threadStackBase;
    
    /* Build linked list of threads for our own use */
    if (threadListHead_lib == NULL) {
        threadListHead_lib = result;
    }
    else {
        result->lib_one = threadListHead_lib;
        threadListHead_lib = result;
    }
    
    if (currScheduler == NULL) {
        defaultScheduler_admit(result);
    }
    else {
        currScheduler->admit(result);
    }
    
    return result->tid;
}

/**
 * Call this from a lwp thread to tell it to DIE
 */
void lwp_exit(void) {
    swap_rfiles(&dummyRFile, &oldRFile);
}

/**
 * Call this from a thread to get its ID
 */
tid_t lwp_gettid(void) {
    if (currentThread == NULL) {
        return NO_THREAD;
    }
    return currentThread->tid;
}

/**
 * Call this from a lwp thread to let the scheduler know we're starting
 *  to block
 */
void  lwp_yield(void) {
    yieldFlag = 1;
    
    swap_rfiles(&(currentThread->state), &oldRFile);
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
 * Remove thread from scheduler or our default sched if there's no scheduler
 */
void lwp_remove_sched_thread(thread victim) {
    if (currScheduler == NULL) {
        defaultScheduler_remove(victim);
    }
    else {
        currScheduler->remove(victim);
    }
}

/**
 * Start all the threads we've lwp_create'd
 */
void  lwp_start(void) {
    thread next = lwp_get_next();
    
    while (next != NULL) {
        currentThread = next;
        swap_rfiles(&oldRFile, &(currentThread->state));
        
        if (stopFlag) {
            stopFlag = 0;
            return;
        }
        
        /* lwp_exit and lwp_yield return back to here */
        if (!yieldFlag) {
            lwp_remove_sched_thread(currentThread);
            
            libraryList_remove(currentThread);
            
            currentThread->tid = -1;
            free(currentThread->stack);
            free(currentThread);
        }
        yieldFlag = 0;
        
        next = lwp_get_next();
    }
}


/**
 * Stop all threads!
 */
void  lwp_stop(void) {
    stopFlag = 1;
    swap_rfiles(&(currentThread->state), &oldRFile);
}

/**
 * Give our lwp system a new scheduler. Rips out all the
 *  old threads from the old scheduler (if any) and puts
 *  them in the new scheduler, then calls init() on the
 *  new scheduler.
 */
void  lwp_set_scheduler(scheduler fun) {
    thread transferringThread = lwp_get_next();
    
    if (currScheduler && currScheduler->shutdown) {
        currScheduler->shutdown();
    }
    
    if (fun == NULL) { /* Revert to default scheduler */
        while (transferringThread != NULL) {
            currScheduler->remove(transferringThread);
            defaultScheduler_admit(transferringThread);
            
            transferringThread = currScheduler->next();
        }
    }
    else {
        /* Switched to a "real" scheduler. Don't keep the old list around */
        while (transferringThread != NULL) {
            lwp_remove_sched_thread(transferringThread);
            fun->admit(transferringThread);
            
            transferringThread = lwp_get_next();
        }
        
        if (fun->init != NULL) {
            fun->init();
        }
        
        threadListHead_sched = threadListTail_sched = NULL;
    }
    
    currScheduler = fun;
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
        return NULL;
    }
    
    while (result != NULL && result->tid != tid) {
        result = result->lib_one;
    }
    
    if (result == NULL) {
        return NULL;
    }
    
    return result;
}
