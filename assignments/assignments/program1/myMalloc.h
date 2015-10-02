//
//  myMalloc.h
//  assignments
//
//  Created by Elliot Fiske on 9/25/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#ifndef myMalloc_h
#define myMalloc_h

#include <stdio.h>

void testLibraries();

/**
 * Defines a header that helps us keep track of the
 *  linked list we create when malloc happens
 */
typedef struct MallocHeader {
    char isFree, isLast;
    struct MallocHeader *next;
    
    size_t dataSize;
    void *data;
} MallocHeader;

#endif /* myMalloc_h */
