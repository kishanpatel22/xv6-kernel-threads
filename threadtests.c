#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "cflags.h"

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
  left_tid  = clone(sort, cstack1 + TSTACK_SIZE, TFLAGS, &left_args);
  right_tid = clone(sort, cstack2 + TSTACK_SIZE, TFLAGS, &right_args);
  
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

#define BAD_ADDRESS         ((void *)0xfffffff)

// although the function exits in thread address space
// the function never gets called since arguments passed 
// to function are invalid and clone system call fails
int 
not_arguments(void *args)
{
  int *ptr = (int *)args;
  *ptr = *ptr + 10;
  exit();
}

// wrong ways to call clone and join system calls test 
int 
wrong_syscall_test()
{   
  int tid, temp = 0;
  
  // passing invalid arguments which are not in address space
  tid = clone(not_arguments, 0, TFLAGS, BAD_ADDRESS);
  if(tid != -1){
    eprintf("wrong system call clone arguments");
  }
  
  // passing invalid function pointer address 
  tid = clone(BAD_ADDRESS, 0, TFLAGS, 0);
  if(tid != -1){
    eprintf("wrong system call clone function pointer");
  }
     
  // passing invalid stack child address 
  tid = clone(not_arguments, BAD_ADDRESS, TFLAGS, &temp);
  if(tid != -1){
    eprintf("wrong system call clone stack address");
  }

  // invalid flag passing 
  

  // join system call for any random thread id
  tid = 1234;
  if(join(tid) != -1){
    eprintf("wrong system call join random thread id");
  }

  // join system call for group leader (thread group leader has tid = -1)
  tid = -1;
  if(join(tid) != -1){
    eprintf("wrong system call join random thread id");
  }
  
  sprintf("wrong system call clone and join");
  // success
  return 0;
}

// ===========================================================================

#define FLAG_STR        "hello"

int fd;

int
open_file_func(void *args)
{
  char buf[128];
  int bytes_read;
    
  // should be able to read from the offset zero 
  bytes_read = read(fd, buf, strlen(FLAG_STR));
  if(bytes_read == 0){
    eprintf("flags test zero bytes read");
  }
  if(strcmp(buf, FLAG_STR) != 0){
    eprintf("flags test incorrect write");
  }  
  exit();
}

int 
child_process_func(void *agrs)
{
  while(1)
      ;
  exit();
}

// clone flags arguments are shown as given below
// TESTCASE: when clone doesn't share the virtual memory 
//         : when clone doesn't inherit the same file descripter table 
//           (creates copy instead)
int 
syscall_flags_test()
{
  int tid, pid;
  fd = open("flags_test.txt", O_RDWR | O_CREATE);
  if(fd == -1){
    eprintf("cannot create file");
  }
  write(fd, FLAG_STR, strlen(FLAG_STR));
  
  // not inheriting open file descripters in clone process 
  tid = clone(open_file_func, 0, CLONE_VM | CLONE_FS | CLONE_THREAD, 0);
  join(tid);
  close(fd);
    
  // not sharing the virtual address space 
  pid = clone(child_process_func, 0, CLONE_FS | CLONE_FILES, 0);
  if(join(pid) != -1){
    eprintf("syscall clone created thread instead of process");
  }
  // kill the child process 
  kill(pid);
  // wait for its completion
  if(pid != wait()) {
    eprintf("clone didn't created a process");
  }

  sprintf("syscall flag test");
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
  
  int nest_func3_tid = clone(nest_func3, cstack + TSTACK_SIZE, TFLAGS, 0);
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
  
  int nest_func2_tid = clone(nest_func2, cstack + TSTACK_SIZE, TFLAGS, 0);
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

  int nest_func1_tid = clone(nest_func1, cstack + TSTACK_SIZE, TFLAGS, 0);
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
      tids[i] = clone(wait_join_func, child_stacks[i] + TSTACK_SIZE, TFLAGS, 0);
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
    thread_pool[i] = clone(incr_global, 0, TFLAGS, 0);
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

int 
peer_fun2(void *agrs) 
{
  sleep(100);
  write(__open_fd__, PEER2_STR, PEER2_STR_LEN);
  exit();
}

int
peer_fun1(void *args) 
{
  sleep(100);
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
peer_relationship_test() 
{
  char buffer[128];
  __open_fd__ = open(PEER_TEST_FILE, O_RDWR | O_CREATE);
  if(__open_fd__ == -1) {
    eprintf("error cannot open file");
  }
  
  __peer_id1__ = clone(peer_fun1, 0, TFLAGS, 0);
  __peer_id2__ = clone(peer_fun2, 0, TFLAGS, 0);
  
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
    clone(clone_without_join_func, 0, TFLAGS, 0);
    clone(clone_without_join_func, 0, TFLAGS, 0);
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
    not_exec_tid  = clone(not_exec_func, 0, TFLAGS, 0);
    exec_tid      = clone(exec_func, 0, TFLAGS, 0);
    
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
    exec_one_tid = clone(exec_one, 0, TFLAGS, 0);
    exec_two_tid = clone(exec_two, 0, TFLAGS, 0);
    
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
  not_fork_func_id = clone(not_fork_func, 0, TFLAGS, 0);
  fork_func_id = clone(fork_func, 0, TFLAGS, 0);
  
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
kill_test() 
{
  global_var = SECRET;
  int new_secret = NEW_SECRET, tid;
  
  // create thread that modifies global variable 
  tid = clone(change_secret, 0, TFLAGS, &new_secret); 
  
  // kill the thread 
  tkill(tid);
  
  // should be able join killed thread
  if(join(tid) == tid && global_var != NEW_SECRET){
    sprintf("thread kill test"); 
  } else{
    eprintf("thread kill test");
  }

  // success
  return 0;
}

// ===========================================================================

#define DURING_EVENT        (5)
#define AFTER_EVENT         (7)
#define VALIDATE_EVENT(x)   (x == (DURING_EVENT * 10 + AFTER_EVENT))

int __event_wait_tid__, __event_tid__;

// function waits for paritcular even to get over 
int
event_wait_func(void *args) 
{
  // suspends the execution event thread completes
  tsuspend(); 
  global_var = global_var * 10 + AFTER_EVENT; 
  exit();
}

// the function completes particular event
int
event_func(void *args)
{
  // does particular event 
  sleep(10);
  global_var = global_var * 10 + DURING_EVENT;
  
  // resumes particular thread which was waiting for event 
  tresume(__event_wait_tid__);
  exit();
}


// threads can suspend their exeuction if required and let other threads
// inform about resuming the exeuction, basically implementation of event wait
// TESTCASE : one thread can suspend its execution and can be resumed by other thread
//          : tsuspend and tresume system calls give signaling mechanism to threads
int 
event_wait_test() 
{
  global_var = 0;
  __event_wait_tid__  = clone(event_wait_func, 0, TFLAGS, 0);
  __event_tid__       = clone(event_func, 0, TFLAGS, 0);
  
  join(__event_tid__);
  join(__event_wait_tid__);
  
  if(VALIDATE_EVENT(global_var)){
    sprintf("event wait test"); 
  } else{
    eprintf("event wait test"); 
  }
  // success 
  return 0;
}

// ===========================================================================

// routiune does infinite recursion and grows thread stack
int 
infinite_recursion(void *agrs) 
{
  infinite_recursion(0);
  exit();
}

// when threads are created without passing stack parameter to clone system
// call the kernel takes care of allocating stack page along with gaurd page
// threads accessing stack address below the stack page must be terminated 
// TESTCASE : an infinite recursion thread must be terminated by xv6
int
stack_smash_test() 
{
  int tid = clone(infinite_recursion, 0, TFLAGS, 0);
  join(tid);
  sprintf("stack smash test");
  // success
  return 0;
}

// ===========================================================================

#define MAX_GROW_PROC_THREADS   (10)
#define NUM_ELEMENTS            (1024)
#define MEM_SIZE                (NUM_ELEMENTS * sizeof(int))

// buffer for holding the allocated memory
int *buffers[MAX_GROW_PROC_THREADS];

void
check_memory_buffer(int *buffer) 
{
    for(int i = 0; i < NUM_ELEMENTS; i++){
       // should be able to dereference memory 
       (*(buffer + i))++;
    }
}

int
allocate_mem_func(void *args)
{
    int i = *((int *)args);
    buffers[i] = malloc(MEM_SIZE);
    exit();
}

int 
grow_proc_test()
{
    int tids[MAX_GROW_PROC_THREADS], args[MAX_GROW_PROC_THREADS];

    for(int i = 0; i < MAX_GROW_PROC_THREADS; i++){
      args[i] = i;
      tids[i] = clone(allocate_mem_func, 0, TFLAGS, args + i);
    }

    for(int i = 0; i < MAX_GROW_PROC_THREADS; i++){
      join(tids[i]); 
    }
    
    // should be able to access the allocate memory by indiviual threads
    for(int i = 0; i < MAX_GROW_PROC_THREADS; i++){
        check_memory_buffer(buffers[i]);
        free(buffers[i]);
    } 
    
    sprintf("grow proc test");

    // success;
    return 0;
}



// ===========================================================================
// ===========================================================================
// ============================ KTHREAD LIBRARY ==============================
// ===========================================================================

// ===========================================================================

#define MAX_THREAD_TEST         (1024)
#define TOO_MANY_THREADS        (100)
#define MAX_THREAD_COUNT        (64 - 3)

int 
kthread_test_func()
{
  sleep(100);
  kthread_exit();
}

// tests for maximum number of threading that library can create 
// TESTCASE : maximum number of threads that can be created 
int 
kthread_lib_max_thread_test() 
{
  printf(1, "STRESS TEST 1 : \n");
  
  kthread_t kth_pool[MAX_THREAD_TEST];
  int thread_count = 0;

  // creating the threads using kthread library
  for(int i = 0; i < TOO_MANY_THREADS; i++){
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
    eprintf("kthread lib max thread ");    
  } 
  sprintf("kthread lib max thread ");    
  // success 
  return 0;
}

// ===========================================================================

#define N                   (6)
#define M                   (2000000)
#define P                   (10)

#define MAGIC_MULTIPLE      (10)

typedef struct matrixargs{
  int **a, **b, **c;  // matrix a, b, and c
  int row;            // i is row of matrix a
  int col;            // j is column of matrix b
  int m;              // num of rows in a = num of columns in b
} matrixargs;

void
print_matrix(int **mat, int n, int m) 
{
  for(int i = 0; i < n; i++){
    for(int j = 0; j < m; j++){
      printf(1, "%d ", mat[i][j]);
    }
    printf(1, "\n");
  }
  printf(1, "\n");
}

int **
get_matrix(int n, int m) {
  int **mat = (int **)malloc(sizeof(int *) * n);
  if(mat == 0){
    eprintf("malloc failed");
  }
  for(int i = 0; i < n; i++) {
    mat[i] = (int *)malloc(sizeof(int) *m);
    if(mat[i] == 0){
      eprintf("malloc failed");
    }
  }
  // initialize all the elements in matrix to 1
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < m; j++) {
      mat[i][j] = 1;
    }
  }
  return mat;
}

void 
free_matrix(int **arr, int n, int m) {
  for(int i = 0; i < n; i++) {
    free(arr[i]);
  }
  free(arr);
}

int
multiply_row_col(void *args)
{
  matrixargs *ptr = (matrixargs *)args;
  int i, j, m;
  i = ptr->row, j = ptr->col;
  m = ptr->m;

  ptr->c[i][j] = 0;
  for(int k = 0; k < m; k++){
    ptr->c[i][j] += ptr->a[i][k] * ptr->b[k][j];
  }
  kthread_exit();
}


// multhreading test gives a stress test for the kthread library which
// implements one to one mapping. The threading library is used for 
// doing matrix multiplcation (C = A x B) by creating some 60 threads
int 
kthread_lib_multithreading_test()
{
  printf(1, "STRESS TEST 2 : \n");

  int **arr = get_matrix(N, M);
  int **brr = get_matrix(M, P);
  int **crr = get_matrix(N, P);

  // arguments to be passed for matrix multiplication
  matrixargs args[N * P], temp; 
  kthread_t kthread_pool[N * P];

  // matrices 
  temp.a = arr;
  temp.b = brr;
  temp.c = crr;
  temp.m = M;
  
  // matrix multiplication using kthreadlibrary
  for(int i = 0; i < N; i++){
    for(int j = 0; j < P; j++){
      args[i * P + j] = temp;
      args[i * P + j].row = i;
      args[i * P + j].col = j;
      if(kthread_create(kthread_pool + i * P + j, multiply_row_col,
                        args + i * P + j) == -1){
        eprintf("multithreading test"); 
      }
    }
  }
  
  // waiting for all threads to complete 
  for(int i = 0; i < N * P; i++){
    if(kthread_join(kthread_pool + i)== -1){
      eprintf("multithreading test"); 
    }
  }

  // verifying if the matrix multiplication is correct 
  for(int i = 0; i < N; i++){
    for(int j = 0; j < P; j++){
      if(crr[i][j] != M){
        eprintf("multithreading test"); 
      }
    }
  }
  
  free_matrix(arr, N, M);
  free_matrix(brr, M, P);
  free_matrix(crr, N, P);

  sprintf("multithreading test");
  // success
  return 0;
}


// ===========================================================================

char *thread_str1 = "abcefg\n";
char *thread_str2 = "xyzlmn\n";

#define THREAD_STR_LEN      (7)

int tfd;
semaphore sem;

int
filewrite_func1(void *args)
{
  semaphore_wait(&sem);    
  for(int i = 0; i < THREAD_STR_LEN; i++) {
    write(tfd, &thread_str1[i], 1);
    sleep(10);
  }
  semaphore_signal(&sem);    
  kthread_exit();
}

int
filewrite_func2(void *agrs)
{
  semaphore_wait(&sem);    
  for(int i = 0; i < THREAD_STR_LEN; i++) {
    write(tfd, &thread_str2[i], 1);
    sleep(10);
  }
  semaphore_signal(&sem);    
  kthread_exit();
}

// the semaphore implementation issues synchronization among threads
// concurrently try to update and modify a shared resource 
// TESTCASE : semaphore ensures mutual execlusion among threads modifying data
int 
kthread_semaphore_test() 
{
  kthread_t th1, th2;
  char str[128];

  // binary semaphore mutex 
  semaphore_init(&sem, 1);

  tfd = open("sem.txt", O_RDWR | O_CREATE); 
  if(tfd == -1){
    eprintf("cannot open file");
  }

  kthread_create(&th1, filewrite_func1, 0);
  kthread_create(&th2, filewrite_func2, 0);
      
  kthread_join(&th1);
  kthread_join(&th2);

  close(tfd);
      
  // verifying if synchronization order
  tfd = open("sem.txt", O_RDONLY); 
  
  // first str1 should be written 
  read(tfd, str, THREAD_STR_LEN);
  str[THREAD_STR_LEN] = '\0';
  if(strcmp(str, thread_str1) != 0 && strcmp(str, thread_str2) != 0){
    eprintf("semaphore test");
  }

  // second str2 should be written 
  read(tfd, str, THREAD_STR_LEN);
  str[THREAD_STR_LEN] = '\0';
  if(strcmp(str, thread_str2) != 0 && strcmp(str, thread_str1) != 0){
    eprintf("semaphore test");
  }
  
  close(tfd);

  sprintf("semaphore test");
  // success 
  return 0;
}

// ===========================================================================

#define VAL1        (10)
#define VAL2        (20)
#define MAX_SIZE    (10)

int race_arr[10], race_index;

// userland spin lock 
struct splock s;

// updates the global array 
int 
update_func1(void *args) 
{
  acquire_splock(&s);
  for(int i = 0; i < MAX_SIZE / 2; i++) {
    race_arr[race_index] = VAL1; 
    sleep(10);
    race_index++;
  }
  release_splock(&s);
  exit();
}

// updates the global array 
int 
update_func2(void *args) 
{
  acquire_splock(&s);
  for(int i = 0; i < MAX_SIZE / 2; i++) {
    race_arr[race_index] = VAL2; 
    sleep(10);
    race_index++;
  }
  release_splock(&s);
  exit();
}

// since we cannot user xv6 provided spinlock which is meant for xv6 kernel
// userland spin lock code was written and function test spin lock functionality 
int 
kthread_uspinlock_test()
{
  kthread_t th1, th2;
  init_splock(&s);
  
  kthread_create(&th1, update_func1, 0);
  kthread_create(&th2, update_func2, 0);
  
  kthread_join(&th1);
  kthread_join(&th2);
  
  // first half of the array must have same value either 1 or 2
  // similarly for the second half of the array values.
  for(int i = 0; i < MAX_SIZE / 2; i++){
    if(race_arr[0] != race_arr[i]){
      eprintf("spinlock test");
    }
  }
  for(int i = MAX_SIZE / 2; i < MAX_SIZE; i++) {
    if(race_arr[MAX_SIZE / 2] != race_arr[i]) {
      eprintf("spinlock test");
    }
  }
  
  sprintf("spinlock test");
  exit();
}

// ===========================================================================

// ===========================================================================

int
main(int argc, char *argv[])
{
    
  // SYSTEM CALL TESTS 
  
  clone_join_test();                  // simple clone and join system call
  wrong_syscall_test();               // wrong ways to call clone and join
  syscall_flags_test();               // flags passed to system call
  nested_clone_join_test();           // nested clone and join system call
  kernel_clone_stack_alloc();         // kernel allocating thread execution stack 
  peer_relationship_test();           // threads sharing peer to peer relationship
  wait_join_test();                   // join and wait both work correctly 
  clone_without_join_test();          // clone thread without join 
  exec_test();                        // exec test for threads
  two_exec_test();                    // exec concurrently done by seperate threads
  fork_test();                        // thread calls fork system call
  kill_test();                        // kills thread 
  event_wait_test();                  // suspend and resume test for threads 
  stack_smash_test();                 // stack smash detection for threads
  grow_proc_test();                   // grow proc tests


  // KTHREAD LIBRARY TESTS
  // stress tests
   
  kthread_lib_max_thread_test();      // max threads created by kthread lib
  kthread_lib_multithreading_test();  // multithreaded program written for test
  kthread_uspinlock_test();           // userland spinlock code test
  kthread_semaphore_test();           // synchorization using semaphore

  exit();
}

