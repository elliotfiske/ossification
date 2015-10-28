//
//  main.c
//  program3
//
//  Created by Elliot Fiske on 10/28/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#define NUM_PHILOSOPHERS 5
#define DEFAULT_NUM_CYCLES 1

void dawdle();
void cleanup();

/**
 * Enum declaring the different states a philosopher can be in.
 */
typedef enum {
    EATING, THINKING, CHANGING
} philState;

/**
 * Structure containing all the necessary parts of being a philosopher.
 */
typedef struct philosopher {
    int id;
    pthread_t threadID;
    philState state;
    int hasLeftFork, hasRightFork;
    
    int cyclesLeft; /* How many more times will I Eat->Think etc. until
                     *  my thread exits? */
} philosopher;

const char *PHILOSOPHER_NAMES = "ABCDEFGHIJKLMNOP"; /* Future-proof in case
                                                     *  philosophers clone
                                                     *  themselves (up to 16
                                                     *  philosophers supported)
                                                     */

pthread_mutex_t forkLocks[NUM_PHILOSOPHERS];
pthread_mutex_t printLock; /* Don't interrupt me when I'm talking! */

philosopher philosophers[NUM_PHILOSOPHERS];

/**
 * Print a bunch of equal signs, so that the cells are separate, but equal.
 */
void divideByEquals() {
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("|=============");
    }
    
    printf("|\n");
}

/**
 * Print the header labels and all the equal signs at the top
 */
void printHeader() {
    int i;
    
    divideByEquals();
    
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("|      %c      ", PHILOSOPHER_NAMES[i]);
    }
    
    printf("|\n");
    
    divideByEquals();
}

/**
 * Some state change happened! Let's print out how everybody's doing.
 */
void printStateChange() {
    int i, forkNdx;
    philosopher currPhil;
    
    pthread_mutex_lock(&printLock);
    
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        currPhil = philosophers[i];
        
        printf("| ");
        
        for (forkNdx = 0; forkNdx < NUM_PHILOSOPHERS; forkNdx++) {
            if (forkNdx == currPhil.hasLeftFork ||
                forkNdx == currPhil.hasRightFork) {
                printf("%d", forkNdx);
            }
            else {
                printf("-");
            }
        }
        
        printf(" ");
        
        if (currPhil.cyclesLeft) {
            switch (currPhil.state) {
                case THINKING:
                    printf("%-6s", "Think");
                    break;
                case EATING:
                    printf("%-6s", "Eat");
                    break;
                case CHANGING:
                default:
                    printf("%-6s", "");
                    break;
            }
        }
        else {
            printf("%-6s", "");
        }
    }
    
    printf("|\n");
    
    pthread_mutex_unlock(&printLock);
}

/**
 * Does the actual work of locking the mutex and grabbing the fork.
 */
void snatchFork(int forkID) {
    pthread_mutex_t *lock = forkLocks + forkID;
    
//    printf("I WANT FORK %d lock addr %zu\n", fcsorkID, (unsigned long) lock);
    
    if(pthread_mutex_lock(lock) != 0) {
        perror("Error grabbing fork\n");
    }
}

/**
 * Grab the left fork, or block until it's available
 */
void takeLeftFork(philosopher *me) {
    int target = me->id;
    
    snatchFork(target);
    me->hasLeftFork = target;
    printStateChange();
}

/**
 * Grab the right fork, or block until it's available
 */
void takeRightFork(philosopher *me) {
    int target = me->id + 1;
    
    if (NUM_PHILOSOPHERS == target) {
        target = 0;
    }
    
    snatchFork(target);
    me->hasRightFork = target;
    printStateChange();
}

/**
 * Om nom nom nom
 */
void eat(philosopher *me) {
    me->state = EATING;
    
    printStateChange();
    
    dawdle();
    me->state = CHANGING;
}

/**
 * Give back the forks buddy, you're done.
 */
void dropForks(philosopher *me) {
    int left = me->id;
    int right = me->id + 1;
    
    if (NUM_PHILOSOPHERS == right) {
        right = 0;
    }
    
    pthread_mutex_unlock(&(forkLocks[left]));
    me->hasLeftFork = -1;
    printStateChange();
    
    pthread_mutex_unlock(&(forkLocks[right]));
    me->hasRightFork = -1;
    printStateChange();
}

/**
 * Think things over.
 */
void think(philosopher *me) {
    me->state = THINKING;
    printStateChange();
    
    dawdle();
    me->state = CHANGING;
}

/**
 * Live the dream as a philosopher! Start off hungry (CHANGING), then EAT
 *  so you have the energy to THINK.
 *
 * The sole argument is the struct containing the philosopher's innards.
 *  Passed as void * because that's how threads work.
 */
void *philosophize(void *id) {
    philosopher *me = &philosophers[*(int *)id]; /* Know thyself */
    
    while (me->cyclesLeft) {
        if (me->id % 2 == 0) {
            takeRightFork(me);
            takeLeftFork(me);
        }
        else {
            takeLeftFork(me);
            takeRightFork(me);
        }
    
        eat(me);
        dropForks(me);
        think(me);
        
        me->cyclesLeft--;
    }
    
    return NULL;
}

/**
 * Determines what the user put as their first command line argument.
 *  If they did something weird, shout at them and exit.
 */
int parseArgumentsToNumCycles(const char * argv[]) {
    long numCycles = strtol(argv[1], NULL, 10);
    int result;
    
    if (errno != 0 || numCycles > INT_MAX) {
        printf("Bad first argument %s. Usage: %s <number of cycles>\n",
               argv[1], argv[0]);
        exit(-1);
    }
    
    return result;
}

/**
 * Create all the philosopher threads and send them on their way.
 */
int main(int argc, const char * argv[]) {
    int i, sys_result;
    int numCycles;
    
    if (argc == 1) {
        numCycles = DEFAULT_NUM_CYCLES;
    }
    else {
        numCycles = parseArgumentsToNumCycles(argv);
    }
    
    sys_result = pthread_mutex_init(&printLock, NULL);
    if (-1 == sys_result) {
        fprintf(stderr, "Init print mutex: %s\n", strerror(errno));
        exit(-1);
    }
    
    printHeader();
    
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        sys_result = pthread_mutex_init(forkLocks + i, NULL);
        if (-1 == sys_result) {
            fprintf(stderr, "Init mutex %i: %s\n", i, strerror(errno));
            exit(-1);
        }
    }
    
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i].id = i;
        philosophers[i].state = CHANGING;
        philosophers[i].hasLeftFork = philosophers[i].hasRightFork = -1;
        philosophers[i].cyclesLeft = numCycles;
        
        sys_result = pthread_create(&(philosophers[i].threadID),
                                      NULL,
                                      philosophize,
                                      &(philosophers[i].id));
        if (-1 == sys_result) {
            fprintf(stderr, "Philosopher %i: %s\n", i, strerror(errno));
            exit(-1);
        }
    }
    
    /* Wait for my babies to come home */
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        sys_result = pthread_join((philosophers[i]).threadID, NULL);
        if (-1 == sys_result) {
            fprintf(stderr, "Join Philosopher %i: %s\n", i, strerror(errno));
            exit(-1);
        }
    }
    
    printStateChange();
    divideByEquals();
    
    cleanup();
    return 0;
}

/**
 * Free up the memory I used. Not mandatory, but polite.
 */
void cleanup() {
    int ndx;
    int sys_result;
    
    for (ndx = 0; ndx < NUM_PHILOSOPHERS; ndx++) {
        sys_result = pthread_mutex_destroy(&(forkLocks[ndx]));
        
        if (sys_result == -1) {
            fprintf(stderr, "Mutex destroy %i: %s\n", ndx, strerror(errno));
            exit(0);
        }
    }
}

/**
 * Thumbs.twiddle()
 */
void dawdle() {
    struct timespec tv;
    int msec = (int) (((double)random()/INT_MAX) * 1000);
    
    tv.tv_sec = 0;
    tv.tv_nsec = 1000000 * msec;
    if (-1 == nanosleep(&tv, NULL)) {
        perror("nanosleep");
    }
}