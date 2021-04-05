#include "types.h"
#include "stat.h"
#include "user.h"

struct myargs {
    int x;
    int y;
    int z;
};

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

int foo(void *args) {
    struct myargs *temp = (struct myargs *)args;
    int a = temp->x;
    int b = temp->y;
    int c = temp->z;
    printf(1, "a = %d, b = %d, c = %d ... foo is over\n", a, b, c);
    return a * b;
}

int main(int argc, char *argv[]) {
    struct myargs temp;
    temp.x = 10;
    temp.y = 20;
    temp.z = 30;
    
    void *child_stack = malloc(4096);
    int tgid = clone(foo, child_stack, 0, (void *)&temp);
    join(tgid);

    printf(1, "Now main over\n");
    free(child_stack);
    exit();
}


