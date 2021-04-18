#include "types.h"
#include "stat.h"
#include "user.h"

// module contains the basic test of clone and join system calls to create threads

#define MAXFIB      (1000)
#define TSTACK      (4096)

struct myargs {
    int x;
    int y;
    int z;
};

int k = 0;

/* O(n^3) algorithm to do nothing */
int waste_cpu_cycles() {
    for(int i = 0; i < MAXFIB; i++) {
        for(int j = 0; j < MAXFIB; j++) {
            for(int k = 0; k < MAXFIB; k++) {
                ; 
            }
        }
    }
    return 0;
}

int bar(void *args) {
    struct myargs *bar_temp = (struct myargs *)args;
    int a, b, c;
    a = bar_temp->x, b = bar_temp->y, c = bar_temp->z;

    waste_cpu_cycles();
    // update the global variable 
    k += a + b + c;

    printf(1, "bar done ...\n");
    // call to exit bar
    exit();
}

int foo(void *args) {
    struct myargs *foo_temp = (struct myargs *)args;
    int a, b, c;
    a = foo_temp->x, b = foo_temp->y, c = foo_temp->z;

    void *child_stack = malloc(TSTACK);
    int tid = clone(bar, (char *)child_stack + TSTACK, 0, args);
    waste_cpu_cycles();
    join(tid);
    free(child_stack);
    
    // update global variable 
    k += a + b + c;

    printf(1, "foo done ...\n");
    // call to exit foo 
    exit();
}

int main(int argc, char *argv[]) {
    struct myargs temp;
    temp.x = 10;
    temp.y = 20;
    temp.z = 30;
    
    void *child_stack = malloc(TSTACK);
    int tid = clone(foo, (char *)child_stack + TSTACK, 0, (void *)&temp);
    join(tid);
    free(child_stack);

    printf(1, "main done ... k = %d\n", k);
    // call to exit for the main function
    exit();
}


