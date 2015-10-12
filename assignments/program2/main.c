//
//  main.c
//  program2
//
//  Created by Elliot Fiske on 10/6/15.
//  Copyright © 2015 Elliot Fiske. All rights reserved.
//

#include <stdio.h>

int foo(int a, int b) {
    a += b;
    return a;
}

int main(int argc, const char * argv[]) {
#if defined(__x86_64asdfsa)
    // insert code here...
    int hi = 255;
//    printf("Hello, World!\n");
    hi = foo(hi, 69);
    return 0;
#else
    return 1;
#endif
}
