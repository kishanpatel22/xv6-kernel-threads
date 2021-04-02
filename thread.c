#include "types.h"
#include "stat.h"
#include "user.h"

int foo(void *args) {
    printf(1, "I reach here !!\n");
    int a = 20;
    int b = 10;
    return a * b;
}

int bar(void *args) {
    int a = 10;
    int b = 20;
    return a + b;
}

int baz(void *args) {
    int a = 10;
    int b = 20;
    return a / b;
}

typedef struct args {
    int x;
    int y;
    int z;
} args;

int main(int argc, char *argv[]) {
    args temp;
    temp.x = 10;
    temp.y = 20;
    temp.z = 30;
    int x = clone(foo, 0, 0, (void *)&temp);
    printf(1, "return from clone = %d\n", x);
    sleep(2);
    exit();
}


