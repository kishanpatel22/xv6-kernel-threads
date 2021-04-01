#include "types.h"
#include "stat.h"
#include "user.h"


int foo(void *args) {
    int a = 20;
    int b = 10;
    printf(1, "%d\n", a + b);
    return a * b;
}

int bar(void *args) {
    int a = 10;
    int b = 20;
    printf(1, "%d\n", a + b);
    return a + b;
}

int baz(void *args) {
    int a = 10;
    int b = 20;
    printf(1, "%d\n", a + b);
    return a / b;
}

int main(int argc, char *argv[]) {
    clone(foo, 0, 0, 0);
    clone(bar, 0, 0, 0);
    clone(baz, 0, 0, 0);
    exit();
}


