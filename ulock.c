#include "types.h"
#include "user.h"

// xv6 provides spin locks only for the kernel modules 
// this is userland implementation of spinlocks 

// The module alos provides implementation of semphores using sping locks


// ==================== USERLAND SPINLOCKS ===================================

void init_splock(splock *s) {
    s->locked = 0;
}

// The code has be taken as it is from xv6 kernel (for implementing userland spin locks)

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

// ===================== QUEUE DATA STRUCTURE ================================

void 
init_queue(queue *q)
{
    q->front = q->rear = 0;
}

int 
isempty(queue q)
{
    return q.front == q.rear;
}

int
enqueue(queue *q, int value)
{
    if((q->front + 1) % MAX_QUEUE_SIZE == q->rear){
        return -1;
    }
    q->arr[q->front] = value;
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    return 0;
}

int
dequeue(queue *q)
{
    if(q->front == q->rear){
        return -1;
    }
    int value = q->arr[q->rear];
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    return value;
}

// ========================== SEMAPHORE =====================================

void 
semaphore_init(semaphore *s, int initval)
{
    s->val = initval;
    init_queue(&(s->q));
    init_splock(&(s->sl));
}

void 
semaphore_wait(semaphore *s) 
{
    acquire_splock(&(s->sl));
    s->val--;
    while(s->val < 0){
        block(s); 
    }
    release_splock(&(s->sl));
}

void
semaphore_signal(semaphore *s) 
{
    acquire_splock(&(s->sl));
    s->val++;
    if(!isempty(s->q)){
        tresume(dequeue(&(s->q))); 
    }
    release_splock(&(s->sl));
}

void 
block(semaphore *s) 
{
    enqueue(&(s->q), gettid());
    release_splock(&(s->sl));
    // make the thread sleep  
    tsuspend();
    acquire_splock(&(s->sl));
}

