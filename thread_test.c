#include "types.h"
#include "stat.h"
#include "user.h"

#define TSTACK_SIZE     (4096)

#define eprintf(fmt, ...)                                                   \
    printf(1, fmt ": failed test case"                                      \
              "\nfile = %s, line number = %d, in function = %s()\n"         \
               ##__VA_ARGS__, __FILE__, __LINE__, __func__);                \
    exit();                                                                 \

#define sprintf(fmt, ...)                                                   \
    printf(1, fmt " : test case passed\n");                                 \
    exit();                                                                 \


// all the users test cases for the xv6 kernel thread implementation 
// test cases are intended to produce results according to the design

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
//           : checks for execution being scheduled for threads by scheduler
//           : checks for join system call blocks the process 
int 
basic_clone_join() 
{
    
    void *cstack1, *cstack2;
    int *arr;
    int n = 100, left_tid, right_tid;
    sortargs left_args, right_args;

    // creating a reverse sorted array
    arr = (int *)malloc(sizeof(int) * n);
    if(!arr) {
        eprintf("malloc failed cannot allocate memory for sorting\n");
    }
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
int _nested_test_global_;

int basic_nest_func3(void *args) {

    int w = 4;
    sleep(3);

    _nested_test_global_ = _nested_test_global_ * 10 + w;
    exit();
}

int basic_nest_func2(void *args) {
    
    int w = 3;
    void *cstack = malloc(TSTACK_SIZE);
    
    int nest_func3_tid = clone(basic_nest_func3, cstack + TSTACK_SIZE, 0, 0);
    sleep(2);
    join(nest_func3_tid);
    free(cstack);
    
    _nested_test_global_ = _nested_test_global_ * 10 + w;
    exit();
}

int basic_nest_func1(void *args) {

    int w = 2; 
    void *cstack = malloc(TSTACK_SIZE);
    
    int nest_func2_tid = clone(basic_nest_func2, cstack + TSTACK_SIZE, 0, 0);
    sleep(1);
    join(nest_func2_tid);
    free(cstack);
    
    _nested_test_global_ = _nested_test_global_ * 10 + w;
    exit();
}

int basic_nested_clone_join() {

    int w = 1; 
    void *cstack = malloc(TSTACK_SIZE);
    
    _nested_test_global_ = 0;

    int nest_func1_tid = clone(basic_nest_func1, cstack + TSTACK_SIZE, 0, 0);
    join(nest_func1_tid);
    free(cstack);

    _nested_test_global_ = _nested_test_global_ * 10 + w;

    if(_nested_test_global_ == 4321) {
        sprintf("nested clone join");
    } else {
        eprintf("nested clone join");
    }
    // success 
    return 0;
}

// ===========================================================================

int
main(int argc, char *argv[])
{
    ///basic_clone_join();                 // does simple clone and join operation
    basic_nested_clone_join();          // does nested clone and join operation
    //kernel_clone_stack_alloc();
    //peer_thread_relationship();  

    exit();
}

