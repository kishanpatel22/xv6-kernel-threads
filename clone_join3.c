#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// the test case is to check for peer to peer relationship among
// the threads. Note any thread created can be joined execept the group leader 
// thread, which is the parent process (main thread)

#define MAXN        (5)

typedef struct myargs {
    int x;
    int y;
    int z;
} myargs;

int k = 0;
int foo_tid, bar_tid;
char str[] = "Programming sometimes is depressing activity.\nSEG FAULT / PAGE FAULT can be as lethal to kernel as covid19 virus !!\n";
int size = sizeof(str) / sizeof(str[0]);

/* I/O time waste */
int io_time_waste(char *s) {
    int fd = open(s, O_RDWR | O_CREATE);
    if(fd < 0) {
        printf(1, "I/O couldn't waste time\n");
        return 0;
    } 
    for(int i = 0; i < MAXN; i++) {
        write(fd, str, size);
    }
    return 0;
}

int bar(void *args) {

    io_time_waste("bar.txt");
    // bar updating global varibale
    k = k + 10;
    printf(1, "bar done ... \n");
    exit();
}

int foo(void *args) {
    
    printf(1, "foo waits for bar\n");
    // wait for bar execution 
    join(bar_tid);
    // foo updating global varibale 
    k = k + 10;
    printf(1, "foo done ... \n");
    exit();
}

int main(int argc, char *argv[]) {
    
    // thread for executing bar
    bar_tid = clone(bar, 0, 0, 0);
    // thread for executing foo
    foo_tid = clone(foo, 0, 0, 0);
    
    printf(1, "main waits for foo\n");
    join(foo_tid);

    printf(1, "main done ... k = %d\n", k);
    // call to exit for the main function
    exit();
}


