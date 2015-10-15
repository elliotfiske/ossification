//
//  main.c
//  program2
//
//  Created by Elliot Fiske on 10/6/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include <stdio.h>
#include "lwp.h"

int foo(int a, int b) {
    a += b;
    return a;
}

//int main(int argc, char *argv[]) {
//    // insert code here...
//    int hi = 255;
//    rfile test;
//
//    swap_rfiles(&test, NULL);
//    
//    hi = foo(hi, 69);
//    return 0;
//}

/**
 * Spawn a sexy new thread with the specified function to run,
 *  arguments for the function, and requested stack size.
 */
tid_t lwp_create(lwpfun functionToRun, void *arguments, size_t stackSize) {
    tid_t result;
    
    fprintf(stderr, "Called lwp_create with size of %zu\n", stackSize);
    
    return result;
}

/**
 * Call this from a lwp thread to tell it to DIE
 */
void  lwp_exit(void) {
    fprintf(stderr, "Called lwp_exit\n");
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
    fprintf(stderr, "Called lwp_start\n");
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