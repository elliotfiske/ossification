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

int main(int argc, char *argv[]) {
    // insert code here...
    int hi = 255;
    rfile test;

    swap_rfiles(&test, NULL);
    
    hi = foo(hi, 69);
    return 0;
}
