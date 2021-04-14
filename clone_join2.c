#include "types.h"
#include "stat.h"
#include "user.h"

// The module basically checks for the clone and join system calls
// functionality which is added in the xv6 kernel.

#define MAXFIB      (1000000)
#define MOD         (1000000007)
#define TSTACK      (4096)

struct myargs {
    int x;
    int y;
    int z;
};

int waste_cpu_cycles() {
    int a = 0, b = 1, c;
    for(int i = 0; i < MAXFIB; i++) {
        c = (a + b) % MOD;
        a = b;
        b = c;
    }
    return c;
}

int foo(void *args) {
    printf(1, "I reach here atleast !!\n");
    struct myargs *foo_temp = (struct myargs *)args;
    int a, b, c;
    a = foo_temp->x, b = foo_temp->y, c = foo_temp->z;
    foo_temp->x++, foo_temp->y++, foo_temp->z++;

    printf(1, "foo done ... a = %d b = %d c = %d\n", a, b, c);
    
    // call to exit foo 
    exit();
}

int main(int argc, char *argv[]) {
    struct myargs temp;
    temp.x = 10;
    temp.y = 20;
    temp.z = 30;
    
    int foo_tgid = clone(foo, 0, 0, (void *)&temp);
    int x = join(foo_tgid);
    printf(1, "main done ... %d with join = %d\n", foo_tgid, x);

    // call to exit for the main function
    exit();
}

