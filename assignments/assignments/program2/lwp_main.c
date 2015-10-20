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
thread currentThread = NULL;
thread threadListTail_sched = NULL; /*Keep track of all threads for scheduler*/
thread threadListHead_sched = NULL;

thread threadListHead_lib = NULL; /* Keep track of all the threads so we can */
                                  /* delete the threads on lwp_stop() */

/* Debug - delete me TODO */
void *muhStack;
int numThreads = 0;


rfile oldRFile, dummyRFile;
scheduler currScheduler = NULL;

unsigned long *oldStackPointer;

int stopPlz = 0;
int yieldPlz = 0;

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
    
    if (victim->tid == -1) {
        printf("WHAT ARE YOU DOING\n");
    }
    
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
 * Spawn a sexy new thread with the specified function to run,
 *  arguments for the function, and requested stack size.
 */
tid_t lwp_create(lwpfun functionToRun, void *arguments, size_t stackSize) {
//    fprintf(stderr, "CALLING LWP_CREATE, hello\n");
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
//        printf("No scheduler available, using defualt\n");
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
    thread dummyThread; /* don't  ask */
    
    swap_rfiles(&dummyRFile, &oldRFile);
    
//    printf("We're still in lwp_exit boss\n");
//    free(currentThread->stack);

    
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
    thread dummyThread;
    yieldPlz = 1;
    
//    fprintf(stderr, "Called lwp_yield\n");
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
 * Start all da threads we've lwp_create'd
 */
void  lwp_start(void) {
//    fprintf(stderr, "CALLING LWP_START heyo\n");
    
    thread next = lwp_get_next();
    
    while (next != NULL) {
        currentThread = next;
        swap_rfiles(&oldRFile, &(currentThread->state));
        
        if (stopPlz) {
            stopPlz = 0;
            return;
        }
        
        /* lwp_exit and lwp_yield return back to here */
        if (!yieldPlz) {
            lwp_remove_sched_thread(currentThread);
            
            libraryList_remove(currentThread);
            
            currentThread->tid = -1;
            free(currentThread->stack);
            free(currentThread);
        }
        yieldPlz = 0;
        
        next = lwp_get_next();
    }
//    printf("Made it back to lwp_start, you know\n");
}


/**
 * Stop all threads!
 */
void  lwp_stop(void) {
    stopPlz = 1;
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
        threadListHead_sched = threadListTail_sched = NULL;
        
        while (transferringThread != NULL) {
            currScheduler->remove(transferringThread);
            fun->admit(transferringThread);
            
            transferringThread = transferringThread->lib_one;
        }
        
        if (fun->init != NULL) {
            fun->init();
        }
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

//void poop(long a) {
//    unsigned long butts = 0xABCDEFAA;
//    unsigned long buttz = 0xE69E69EE;
//    
//    printf("Hi I'm in here now %zu arg: %zu\n", butts, a);
//    
//    printf("STuff %zu\n", buttz);
//    
//    printf("HI there you\n");
//    
//    printf("HI there you\n");
//    printf("yielding\n");
//    lwp_yield();
//    printf("HI there you\n");
//    
//    return;
//}
//
//
//int main(int argc, char *argv[]) {
//    long argument = 69;
//    
//    lwp_create((lwpfun)poop, (void *)argument, 1000);
////    defaultScheduler_admit(toRun);
//    
//    lwp_start();
//    printf("I MADE IT BACK!!\n");
//    
//    return 0;
//}

#define STACKSIZE   4096
#define THREADCOUNT 4096
#define TARGETCOUNT 50000

int count;
int yieldcount;

static void body(int num)
{
    int i;
    
    for(;;) {
        int num = random()&0xF;
        for(i=0;i<num;i++) {
            yieldcount++;
            lwp_yield();
        }
        if ( count < TARGETCOUNT )
            count++;
        else
            lwp_exit();
    }
}


int main() {
    int i;
    
    srandom(0);                   /* There's random and there's random... */
    printf("Spawining %d threads.\n", THREADCOUNT);
    count=0;
    for(i=0;i<THREADCOUNT;i++)
        lwp_create((lwpfun)body,(void*)0,STACKSIZE);
    lwp_start();
    printf("Done.  Count is %d. (Yielded %d times)\n", count, yieldcount);
    exit(0);
}