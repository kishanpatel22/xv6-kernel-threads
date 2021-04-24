#include "types.h"
#include "user.h"

// xv6 provides spin locks only for the kernel modules 
// this is userland implementation of spinlocks 

// The module alos provides implementation of semphores using sping locks


// ==================== USERLAN SPINLOCKS ===================================

void init_splock(splock *s) {
    s->locked = 0;
}

// atomic test and set hardware based instruction 
static inline uint
xchg(volatile uint *addr, uint newval)
{
  uint result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

// aquire a userland spin lock 
void 
acquire_splock(splock *s) 
{
    // The xchg is atomic.
    while(xchg(&(s->locked), 1) != 0)
        ;
    return;
}

// relase a userland spin lock 
void 
release_splock(splock *s) 
{
     __sync_synchronize();

    // release the lock, equivalent to lk->locked = 0.
    // this code can't use a c assignment, since it might
    // not be atomic. a real os would use c atomics here.
    asm volatile("movl $0, %0" : "+m" (s->locked) : );
    return;
}

// ========================== SEMAPHORE =====================================

void 
init_queue(queue *q)
{
    q->front = q->rear = 0;
}

void
enqueue(queue *q, int value)
{
    q->arr[q->front] = value;
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
}

int
dequeue(queue *q)
{
    if(q->front == q->rear) {
        return -1;
    }
    int value = q->arr[q->rear];
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    return value;
}


