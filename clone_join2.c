#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// The module basically checks for the clone and join system calls
// functionality which is added in the xv6 kernel.

#define MAXFIB      (10000)
#define MOD         (100000007)

typedef struct myargs {
    int x;
    int y;
    int z;
} myargs;

int k = 0;
char *argv[] = { "cat", "temp.txt", 0 };

void print_fun(void) {
}

int waste_cpu_cycles() {
    int a = 0, b = 1, c;
    for(int i = 0; i < MAXFIB; i++) {
        c = (a + b) % MOD;
        a = b;
        b = c;
        k++;
    }
    return c;
}

int bar(void *args) {
    waste_cpu_cycles();
    exit();
}

int foo(void *args) {
    waste_cpu_cycles();
    printf(1, "THis works every system call works prefects !! \n");
    myargs *ptr = (myargs *)args;
    k = ptr->x + ptr->y + ptr->z;
    exit();
}

int main(int argc, char *argv[]) {
    k = 0;
    myargs temp;
    temp.x = 10;
    temp.y = 20;
    temp.z = 30;
    clone(foo, 0, 0, &temp);
    join();

    printf(1, "main done ... k = %d\n", k);
    // call to exit for the main function
    exit();
}

