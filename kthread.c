#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "cflags.h"

// kthreads user library implementation 
#define KERNEL_STACK_ALLOC      (1)
#define SBRK_STACK_ALLOC        (2)

// change the macro to change the implementation of the library
#define LIB_IMPLEMENTATION      (KERNEL_STACK_ALLOC)

#define KTHREAD_STACK_SIZE      (4096)

#define BASE_ADDRESS(stack)     ((stack) + KTHREAD_STACK_SIZE)
#define START_ADDRESS(stack)    ((stack) - KTHREAD_STACK_SIZE)


// module contains userland threading library which creates
// threads using the underlying system calls like clone and join

// creates thread stack using heap memory allocation
int 
create_thread_stack(void **stack) 
{
 
    // kthread library manages thread stack allocation
    #if LIB_IMPLEMENTATION == SBRK_STACK_ALLOC
   
    *stack = malloc(KTHREAD_STACK_SIZE);
    // malloc fails
    if(*stack == 0) {
        return -1;
    }

    // the stack address is the base address 
    *stack = BASE_ADDRESS(*stack);
    
    // kernel manages the thread stack allocation
    #else
        *stack = 0;
    #endif 

    // success 
    return 0;
}

// destory the stack allocated for thread 
void
destory_thread_stack(void **stack) 
{
    // kthread library manages thread stack allocation
    #if LIB_IMPLEMENTATION == SBRK_STACK_ALLOC
    
    // deallocate memmory of thread stack
    free(START_ADDRESS(*stack));
    *stack = 0;
    
    #endif 

    return;
}


// creates the threads using clone system call implementation 
// returns 0 if successfully creates thread else it returns -1 
int 
kthread_create(kthread_t *kthread, int func(void *args), void *args)
{
    kthread->state = NEW;
    
    // allocate child stack 
    if(create_thread_stack(&(kthread->tstack)) == -1) {
        return -1;
    }
    
    // cannot create thread 
    if((kthread->tid = clone(func, kthread->tstack, TFLAGS, args)) == -1) {
        destory_thread_stack(&(kthread->tstack));
        kthread->state = DEAD;
        return -1;
    }

    // thread is running 
    kthread->state = RUNNING;
    return 0;
}

// join the thread in thread group
int 
kthread_join(kthread_t *kthread) 
{
    // thread has already died 
    if(kthread->state == DEAD) {
        return -1;
    }
    
    // join system call joining the thread 
    int jtid = join(kthread->tid);

    // destory the stack allocated for the thread 
    destory_thread_stack(&(kthread->tstack));

    kthread->state = DEAD;
    return jtid;
}

// cancel thread in thread group
void
kthread_cancel(kthread_t *kthread) {
    
    // thread has already died 
    if(kthread->state == DEAD) {
        return;
    }
    
    // kill the thread execution 
    kill(kthread->tid);

    // deallocate stack of thread exeuction 
    destory_thread_stack(&(kthread->tstack));
    
    // thread has died 
    kthread->state = DEAD;

    return;
}


// kthread exit for the thread routine 
int 
kthread_exit() 
{
    // exits handles dealloction of stack 
    exit();
}


