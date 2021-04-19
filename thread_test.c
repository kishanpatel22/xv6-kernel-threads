#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define TSTACK_SIZE     (4096)

#define eprintf(fmt, ...)                                                   \
    printf(1, fmt ": failed test case"                                      \
              "\nfile = %s, line number = %d, in function = %s()\n"         \
               ##__VA_ARGS__, __FILE__, __LINE__, __func__);                \
    exit();                                                                 \

#define sprintf(fmt, ...)                                                   \
    printf(1, fmt " : test case passed\n");                                 \


// all functions provide test cases for the xv6 kernel thread implementation 
// test cases are intended to produce appropriate results which can be verified.

int global_var;     // one global variable for all functions to test 

// ===========================================================================

typedef struct sortargs {
    int *arr, start, end;
} sortargs;

// merge routine needed for merging two sorted arrays
void 
merge(int *arr, int start, int mid, int end) 
{
    int n1 = mid - start + 1;
    int n2 = end - mid;
    int left[n1 + 1], right[n2 + 1];
    for(int i = 0; i < n1; i++) {
        left[i] = arr[start + i];
    }
    for(int i = 0; i < n2; i++) {
        right[i] = arr[mid + i + 1];
    }
    left[n1] = right[n2] = (int)10e9;
    int i = 0, j = 0;
    for(int k = start; k <= end; k++) {
        if(left[i] <= right[j]) {
            arr[k] = left[i];
            i++;
        } else {
            arr[k] = right[j];
            j++;
        }
    }
    return;
}

// merge sort for sorting two arrays
void 
mergesort(int *arr, int start, int end) 
{
    if(start < end) {
        int mid = start + (end - start) / 2;
        mergesort(arr, start, mid);
        mergesort(arr, mid + 1, end);
        merge(arr, start, mid, end);
    }
}

// thread routine calls merge sort for sorting
int 
sort(void *args) 
{
    sortargs *ptr = (sortargs *)args; 
    mergesort(ptr->arr, ptr->start, ptr->end);
    exit();
}


// does clone system call for creating threads and waits using join system 
// creates two threads and waits for concurrent execution of merge sort
// TEST CASE : checks virtual address space is shared (heap and text/data)
//           : checks for threads being executed concurrently (as if were a process)
//           : check for join system call which blocks/suspends the process 
int 
basic_clone_join() 
{
    
    void *cstack1, *cstack2;
    int *arr;
    int n = 100, left_tid, right_tid;
    sortargs left_args, right_args;

    arr = (int *)malloc(sizeof(int) * n);
    if(!arr) {
        eprintf("malloc failed cannot allocate memory for sorting\n");
    }
    // creating a reverse sorted array
    for(int i = 0; i < n; i++) {
        arr[i] = n - i;             
    }

    // arguments for thread execution 
    left_args.arr = right_args.arr = arr;
    left_args.start = 0;
    left_args.end = n / 2;
    right_args.start = (n / 2) + 1;
    right_args.end = n - 1;
    
    // stacks allocated for execution
    cstack1 = malloc(TSTACK_SIZE);
    cstack2 = malloc(TSTACK_SIZE);
    if(!cstack1 || !cstack2) {
        eprintf("malloc failed cannot allocate memory for stack space"); 
    }

    // creating threads for sorting concurrently 
    left_tid  = clone(sort, cstack1 + TSTACK_SIZE, 0, &left_args);
    right_tid = clone(sort, cstack2 + TSTACK_SIZE, 0, &right_args);
    
    join(left_tid);         // wait for left array to be sorted
    join(right_tid);        // wait for right array to be sorted
    
    // merge the sorted arrays 
    merge(arr, 0, n / 2, n - 1);
    
    // verifying if the array is sorted 
    for(int i = 0; i < n; i++) {
        if(arr[i] != i + 1) {
            eprintf("basic clone join");
        }
    }
    sprintf("basic clone join");

    free(arr);
    free(cstack1);
    free(cstack2);

    // success
    return 0;
}

// ===========================================================================

int 
basic_nest_func3(void *args) 
{
    int w = 4;
    sleep(3);

    global_var = global_var * 10 + w;
    exit();
}

int
basic_nest_func2(void *args) 
{
    int w = 3;
    void *cstack = malloc(TSTACK_SIZE);
    
    int nest_func3_tid = clone(basic_nest_func3, cstack + TSTACK_SIZE, 0, 0);
    sleep(2);
    join(nest_func3_tid);
    free(cstack);
    
    global_var = global_var * 10 + w;
    exit();
}

int
basic_nest_func1(void *args) 
{
    int w = 2; 
    void *cstack = malloc(TSTACK_SIZE);
    
    int nest_func2_tid = clone(basic_nest_func2, cstack + TSTACK_SIZE, 0, 0);
    sleep(1);
    join(nest_func2_tid);
    free(cstack);
    
    global_var = global_var * 10 + w;
    exit();
}

// basic nested clone join test for clone system call created threads again 
// creating threads for execution and waiting for its execution
// TEST CASE : checks for threads can call clone system call 
//           : indirectly gives test for threads can make system call and block
int 
basic_nested_clone_join() 
{
    int w = 1; 
    void *cstack = malloc(TSTACK_SIZE);
    
    global_var = 0;

    int nest_func1_tid = clone(basic_nest_func1, cstack + TSTACK_SIZE, 0, 0);
    join(nest_func1_tid);
    free(cstack);

    global_var = global_var * 10 + w;

    if(global_var == 4321) {
        sprintf("nested clone join");
    } else {
        eprintf("nested clone join");
    }
    // success 
    return 0;
}

// ===========================================================================

#define MAX_ITERATIONS  (10000)

int increament_global(void *args) {
    for(int i = 0; i < MAX_ITERATIONS; i++) {
        global_var++;
    }
    exit();
}

// the basic clone and join system calls, with passing child stack parameter
// the kernel allocates pages for stack, along with taking care of guard page
// kernel basically extends/grows the virtual address space of shared memory 
// TEST CASE : check if kernel allocates stack 
//           : creating thread pools for execution
int 
kernel_clone_stack_alloc() 
{
    // thread pool for storing thread ids
    int thread_pool[5];
    // initializing global variables 
    global_var = 0;
    
    // create threads and execution begins concurrently 
    for(int i = 0; i < 5; i++) {
        thread_pool[i] = clone(increament_global, 0, 0, 0);
    }
    // join all the threads i.e. wait for its execution
    for(int i = 0; i < 5; i++) {
        join(thread_pool[i]);
    }
    if(global_var == 5 * MAX_ITERATIONS) {
        sprintf("kernel clone stack allocation");
    } else {
        eprintf("kernel clone stack allocation");
    }
    // sucess
    return 0;
}

// ===========================================================================

int __open_fd__, __peer_id1__, __peer_id2__;

#define PEER1_STR       "peer 1 : SEG FAULT / PAGE FAULT can be very lethal to programmers\n"
#define PEER2_STR       "peer 2 : programming is sometimes depressing\n"

#define PEER1_STR_LEN   (66)
#define PEER2_STR_LEN   (45)

int peer_fun2(void *agrs) {
    sleep(2);
    write(__open_fd__, PEER2_STR, PEER2_STR_LEN);
    exit();
}

int peer_fun1(void *args) {
    sleep(2);
    // waiting for peer 2 to complete (although peer 1 is not parent)
    join(__peer_id2__);
    write(__open_fd__, PEER1_STR, PEER1_STR_LEN);
    exit();
}

// clone unlike fork doesn't enforces any parent child relationship.
// all the threads share peer to peer relationship with one group leader thread
// the group leader thread is speacial thread which cannot be joined, rest 
// other threads can be joined by any other exitising threads
// TESTCASE : any thread can wait to join for any other thread in thread pool
//          : open file descripter are shared in thread creating 
int 
thread_peer_relationship() 
{
    char buffer[128];
    __open_fd__ = open("peer.txt", O_RDWR | O_CREATE);
    if(__open_fd__ == -1) {
        eprintf("error cannot open file");
    }
    __peer_id1__ = clone(peer_fun1, 0, 0, 0);
    __peer_id2__ = clone(peer_fun2, 0, 0, 0);
    
    // just waiting for peer 1 to complete 
    join(__peer_id1__);
    close(__open_fd__);
    
    // verifying the correctedness 
    __open_fd__ = open("peer.txt", O_RDWR | O_CREATE);
    if(__open_fd__ == -1) {
        eprintf("error cannot open file");
    }
    
    read(__open_fd__, buffer, PEER2_STR_LEN); 
    buffer[PEER2_STR_LEN] = '\0';
    
    if(strcmp(buffer, PEER2_STR) != 0) {
        eprintf("thread peer relationship"); 
    }
    
    read(__open_fd__, buffer, PEER1_STR_LEN); 
    buffer[PEER1_STR_LEN] = '\0';
    if(strcmp(buffer, PEER1_STR) != 0) {
        eprintf("thread peer relationship"); 
    }

    sprintf("thread peer relationship");

    // succuess
    return 0;
}

// ===========================================================================

int fork_test() {
     
    
    // success 
    return 0;
}


int exec_test() {
   

    // success 
    return 0;
}


// ===========================================================================

int
main(int argc, char *argv[])
{
    //basic_clone_join();                 // simple clone and join system call
    //basic_nested_clone_join();          // nested clone and join system call
    //kernel_clone_stack_alloc();         // kernel allocating thread execution stack 
    //thread_peer_relationship();         // threads sharing peer to peer relationship
    fork_test();                        // fork test for threads
    exec_test();                        // exec test for threads

    exit();
}


