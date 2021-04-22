#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define TSTACK_SIZE     (4096)

#define eprintf(fmt, ...)                                                   \
    printf(1, fmt ": FAILED"                                                \
              "\nfile = %s, line number = %d, in function = %s()\n"         \
               ##__VA_ARGS__, __FILE__, __LINE__, __func__);                \
    exit();                                                                 \

#define sprintf(fmt, ...)                                                   \
    printf(1, fmt " : PASSED\n");                                           \


// all functions provide test cases for the xv6 kernel thread implementation 
// test cases are intended to produce appropriate results which can be verified.

int global_var;     // one global variable for all functions to test 

// ===========================================================================
// =========================== SYSTEM CALLS ==================================
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
//           : check for join system call which blocks/suspends the thread
int 
clone_join_test() 
{
    
    void *cstack1, *cstack2;
    int *arr;
    int n = 100, left_tid, right_tid;
    sortargs left_args, right_args;

    arr = (int *)malloc(sizeof(int) * n);
    if(!arr) {
        eprintf("malloc failed");
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
        eprintf("malloc failed"); 
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
            eprintf("clone join test");
        }
    }
    sprintf("clone join test");

    free(arr);
    free(cstack1);
    free(cstack2);

    // success
    return 0;
}

// ===========================================================================

int 
nest_func3(void *args) 
{
    int w = 4;
    sleep(3);

    global_var = global_var * 10 + w;
    exit();
}

int
nest_func2(void *args) 
{
    int w = 3;
    void *cstack = malloc(TSTACK_SIZE);
    
    int nest_func3_tid = clone(nest_func3, cstack + TSTACK_SIZE, 0, 0);
    sleep(2);
    join(nest_func3_tid);
    free(cstack);
    
    global_var = global_var * 10 + w;
    exit();
}

int
nest_func1(void *args) 
{
    int w = 2; 
    void *cstack = malloc(TSTACK_SIZE);
    
    int nest_func2_tid = clone(nest_func2, cstack + TSTACK_SIZE, 0, 0);
    sleep(1);
    join(nest_func2_tid);
    free(cstack);
    
    global_var = global_var * 10 + w;
    exit();
}

// nested clone join test for clone system call created threads again 
// creating threads for execution and waiting for its execution
// TEST CASE : checks for threads can call clone system call 
//           : indirectly gives test for threads can make system call and block
int 
nested_clone_join_test() 
{
    int w = 1; 
    void *cstack = malloc(TSTACK_SIZE);
    
    global_var = 0;

    int nest_func1_tid = clone(nest_func1, cstack + TSTACK_SIZE, 0, 0);
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

// Hardy-Ramanujan number 
#define SECRET_KEY          (1729)          
#define MAX_CHILD_THREADS   (3)
#define CHECK(key)          (key == MAX_CHILD_THREADS * SECRET_KEY)

int __key__;

int 
wait_join_func(void *agrs) 
{
    sleep(5); 
    __key__ += SECRET_KEY;
    exit();
} 

// join waits for a paritcular thread to finish, wait must wait for "any" 
// child process to finish. Note child process will be suspended uptill
// execution of all the threads which are present in thread group of child.
// TEST CASE : check if wait returns only after all threads are completed
//           : globals being shared in fork and child threads 
int 
wait_join_test() 
{
    
    int pid, wpid;
    int tids[MAX_CHILD_THREADS];
    void *child_stacks[MAX_CHILD_THREADS];

    // create a child process 
    pid = fork();
    if(pid == 0){
        // child process creates few threads
        for(int i = 0; i < MAX_CHILD_THREADS; i++) {
            child_stacks[i] = malloc(TSTACK_SIZE);
            tids[i] = clone(wait_join_func, child_stacks[i] + TSTACK_SIZE, 0, 0);
        } 
        // child process waits for all threads to be joined 
        for(int i = 0; i < MAX_CHILD_THREADS; i++) {
            join(tids[i]);
            free(child_stacks[i]);
        }
        // were the threads really working 
        if(!CHECK(__key__)) {
            eprintf("wait join test\n"); 
        }
        exit();
    } 
    
    // parent process should for child process + child threads to finish
    wpid = wait();
    if(wpid == pid) {
        sprintf("wait join test");
    } else {
        eprintf("wait join test");
    }

    // sucess 
    return 0;
}

// ===========================================================================

#define MAX_ITERATIONS   (10000)
#define MAX_THREAD_POOL  (5)

int
incr_global(void *args) 
{
    for(int i = 0; i < MAX_ITERATIONS; i++) {
        global_var++;
    }
    exit();
}

// the clone and join system calls "without passing child stack parameter"
// the kernel allocates pages for stack, along with taking care of guard page
// kernel basically extends/grows the virtual address space of shared memory 
// TEST CASE : check if kernel allocates stack 
//           : creating thread pools for execution
int 
kernel_clone_stack_alloc() 
{
    // thread pool for storing thread ids
    int thread_pool[MAX_THREAD_POOL];
    // initializing global variables 
    global_var = 0;
    
    // create threads and execution begins concurrently 
    for(int i = 0; i < MAX_THREAD_POOL; i++) {
        thread_pool[i] = clone(incr_global, 0, 0, 0);
    }
    // join all the threads i.e. wait for its execution
    for(int i = 0; i < MAX_THREAD_POOL; i++) {
        join(thread_pool[i]);
    }
    if(global_var == MAX_THREAD_POOL * MAX_ITERATIONS) {
        sprintf("kernel clone stack allocation");
    } else {
        eprintf("kernel clone stack allocation");
    }
    // sucess
    return 0;
}

// ===========================================================================

int __open_fd__, __peer_id1__, __peer_id2__;

#define PEER_TEST_FILE  "peer_test.txt"

#define PEER1_STR       "peer 1 : SEG FAULT / PAGE FAULT can be very lethal to programmers\n"
#define PEER2_STR       "peer 2 : programming is sometimes depressing\n"

#define PEER1_STR_LEN   (66)
#define PEER2_STR_LEN   (45)

int peer_fun2(void *agrs) 
{
    sleep(2);
    write(__open_fd__, PEER2_STR, PEER2_STR_LEN);
    exit();
}

int peer_fun1(void *args) 
{
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
thread_peer_relationship_test() 
{
    char buffer[128];
    __open_fd__ = open(PEER_TEST_FILE, O_RDWR | O_CREATE);
    if(__open_fd__ == -1) {
        eprintf("error cannot open file");
    }
    __peer_id1__ = clone(peer_fun1, 0, 0, 0);
    __peer_id2__ = clone(peer_fun2, 0, 0, 0);
    
    // just waiting for peer 1 to complete 
    join(__peer_id1__);
    close(__open_fd__);
    
    // verifying the correctedness 
    __open_fd__ = open(PEER_TEST_FILE, O_RDWR | O_CREATE);
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

int 
clone_without_join_func(void *agrs) 
{
    sleep(10);
    exit();
}

// clone system call done without a join system call, i.e. any process or
// group leader thread exits before the other threads are joined or completed
// TEST CASE : waid only returns after the child process all threads are killed
//           : clone without join will kill all threads in exit
int 
clone_without_join_test() 
{
    int pid, wpid;
    pid = fork();
    // child create threads and doesn't wait
    if(pid == 0) {
        // creating threads 
        clone(clone_without_join_func, 0, 0, 0);
        clone(clone_without_join_func, 0, 0, 0);
        // calling exit not waiting for the threads
        exit();
    } 
    // parent waits for the child
    wpid = wait(); 
    if(wpid == pid) {
        sprintf("clone without join");
    } else {
        eprintf("clone without join"); 
    }
    // success
    return 0;
}


// ===========================================================================

char *exec_argv[] = {"echo", "exec test PASSED", 0};

int 
exec_func(void *args) 
{
    // does an exec system call
    exec(exec_argv[0], exec_argv);
    // exec should ideally not return 
    eprintf("exec test");
    exit(); 
}

int 
not_exec_func(void *agrs) 
{
    // thread simply sleeps
    sleep(10);
    exit();
}

// created cloned process makes an exec, which will kill all the threads
// running for the currently executing 
// TEST CASE : exec system call inside a thread, replaces virtual address space 
//             of whole process along with all it's threads
//           : all threads of the process die execpt the group leader
int
exec_test() 
{
    int pid, exec_tid, not_exec_tid;
    pid = fork();
    // child creates two threads and one of thread does exec and other sleeps for 10 sec
    if(pid == 0) {
        
        // create two threads one doesn't do exec and one does exec 
        not_exec_tid = clone(not_exec_func, 0, 0, 0);
        exec_tid      = clone(exec_func, 0, 0, 0);
        
        // joining the exec thread
        join(not_exec_tid);
        join(exec_tid);
        
        // join should never returns 
        eprintf("exec test");
    }
    // parent simply waits for the child
    else {
        int cpid = wait();
        if(cpid == pid) {
            sprintf("exec test");
        } else {
            eprintf("exec test");
        }
    }
    // success 
    return 0;
}

// ===========================================================================

char *exec_one_argv[] = {"echo", "two exec test PASSED", 0};
char *exec_two_argv[] = {"echo", "two exec test FAILED", 0};

int 
exec_one(void *args) 
{
    exec(exec_one_argv[0], exec_one_argv);
    // exec should never return
    exit();
}

int 
exec_two(void *agrs) 
{
    exec(exec_two_argv[0], exec_two_argv);
    // exec should never return
    exit();
}

// when more than one threads concurrently tries make exec system call 
// then only one of the thread is given chance to complete exec based upon 
// which threads the kernel scheduler had scheduled. Basically exec system 
// call will execute based upon first come first serve.
// TEST CASE : synchronized update of the virtual address space.
int 
two_exec_test() 
{
    int pid, exec_one_tid, exec_two_tid, wpid;
    pid = fork();
    // child process creates two threads both doing exec concurrently 
    if(pid == 0) {

        // create two threads for making exec system call
        exec_one_tid = clone(exec_one, 0, 0, 0);
        exec_two_tid = clone(exec_two, 0, 0, 0);
        
        // wait for the threads to exec
        join(exec_one_tid);
        join(exec_two_tid);

        // join should ideally not return
        eprintf("two_exec_test"); 
        exit();
    } 

    // parent waits for child process to complete only on exec.
    wpid = wait();
    if(wpid == pid) {
        sprintf("two exec test");
    } else {
        eprintf("two exec test");
    }
    
    // success
    return 0;
}

// ===========================================================================

#define FORK_TEST_FILE          "fork_test.txt"
#define FORK_STR                "foobarbaz"
#define FORK_STR_LEN            (9) 
#define FORK_SECRET             (196)
    
int fork_func_id, not_fork_func_id;

// thread simply sleeps 
int 
not_fork_func(void *agrs) 
{
    sleep(50);
    exit();
}

// creates child process and waits for it's execution 
int 
fork_func(void *agrs) 
{
    int pid, wpid, fd;
    char buf[FORK_STR_LEN + 1];
    
    // create identical child process  
    pid = fork();
    
    if(pid == -1){
        eprintf("fork test cannot make system call fork");
    }

    // child does write system call to make change in file system.
    if(pid == 0){
        
        fd = open(FORK_TEST_FILE, O_RDWR | O_CREATE);
        write(fd, FORK_STR, FORK_STR_LEN);
        close(fd);
        
        // child process doesn't have other thread in address space 
        // join must fail since thread doens't belong to group
        if(join(not_fork_func_id) != -1){
            eprintf("fork test failed join should not happend");
        }

        exit();
    }

    // thread waits for child process to exit
    wpid = wait(); 
    if(wpid == pid){
        
        // reads the file modified by the child process 
        fd = open(FORK_TEST_FILE, O_RDONLY);
        read(fd, buf, FORK_STR_LEN);
        buf[FORK_STR_LEN] = '\0';

        // compare the content insider file
        if(strcmp(buf, FORK_STR) == 0) {
            sprintf("fork test"); 
        } else {
            eprintf("fork test child process not working correctly"); 
        }
        close(fd);

    } else {
        eprintf("fork test wait and not working");
    }
    exit();
}

// thread making fork system call, creates new process with the only thread 
// executing for newly created process will be thread which called fork.
// TEST CASE : any thread can create new process using fork (with only one identical thread )
//           : any thread can wait for the child process 
//           : others thread apart for thread calling fork are never duplicated 
int 
fork_test() 
{
    // creates thread one for executing fork and one for increamenting global variable
    not_fork_func_id = clone(not_fork_func, 0, 0, 0);
    fork_func_id = clone(fork_func, 0, 0, 0);
    
    // join thread 
    join(fork_func_id);
    join(not_fork_func_id);

    // success 
    return 0;
}

// ===========================================================================

#define SECRET              (100)
#define NEW_SECRET          (200)

// the function runs infinetly 
int
change_secret(void *args) 
{   
    for(;;){
        ;
    }
    global_var = NEW_SECRET;
    exit();
}

// threads in the same thread group can kill each other.
// however any thread cannot kill the thread group leader.
// TEST CASE : main thread killing a peer thread
int 
thread_kill_test() 
{
    global_var = SECRET;
    int new_secret = NEW_SECRET, tid;
    
    // create thread that modifies global variable 
    tid = clone(change_secret, 0, 0, &new_secret); 
    // kill the thread 
    tkill(tid);
    
    // should be unable to join killed thread
    if(join(tid) == -1 && global_var != NEW_SECRET){
        sprintf("thread kill test"); 
    } else{
        eprintf("thread kill test");
    }
    exit();
}

// ===========================================================================
// ============================ KTHREAD LIBRARY ==============================
// ===========================================================================

// ===========================================================================

#define MAX_THREAD_TEST         (1024)
#define MAX_THREAD_COUNT        (64 - 3)

int 
kthread_test_func()
{
    sleep(10);
    kthread_exit();
}

// tests for maximum number of threading that library can create 
// TESTCASE : maximum number of threads that can be created 
int 
kthread_lib_test() 
{
    kthread_t kth_pool[MAX_THREAD_TEST];
    int thread_count = 0;

    // creating the threads using kthread library
    for(int i = 0; i < MAX_THREAD_TEST; i++){
        if(kthread_create(kth_pool + i, kthread_test_func, 0) == -1){
            break; 
        }
        thread_count++;
    }
    
    // join the maximum threads that are created
    for(int i = 0; i < thread_count; i++){
        kthread_join(kth_pool + i);
    }

    // check for number of threads created 
    if(thread_count < MAX_THREAD_COUNT) {
        eprintf("kthread lib test");
    } 
    
    sprintf("kthread lib test");
    // success 
    return 0;
}

// ===========================================================================

int
kthread_attach_detach_test()
{
    
    // success
    return 0;
}

// ===========================================================================

int
main(int argc, char *argv[])
{
    
    // SYSTEM CALL TESTS 
    
    //clone_join_test();                  // simple clone and join system call
    //nested_clone_join_test();           // nested clone and join system call
    //kernel_clone_stack_alloc();         // kernel allocating thread execution stack 
    //thread_peer_relationship_test();    // threads sharing peer to peer relationship
    //wait_join_test();                   // join and wait both work correctly 
    //clone_without_join_test();          // clone thread without join 
    //exec_test();                        // exec test for threads
    //two_exec_test();                    // exec concurrently done by seperate threads
    //fork_test();                        // thread calls fork system call
    //thread_kill_test();                 // kills thread 
    
    
    // KTHREAD LIBRARY TESTS
    //kthread_lib_test();                 // max threads created by kthread lib
    kthread_attach_detach_test();         // kthread attach detach

    // SYNCHRONIZATION ISSUES AND SOLUTIONS
    
    exit();
}



