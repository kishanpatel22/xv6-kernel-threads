#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}


// sys_clone function copies the arguments to clone from the user process
// address space to the xv6 kernel memory. The function after copying
// the values passes the control to clone function
int sys_clone(void) {
    
    // actual clone call prototype : 
    // clone(int (*fun)(void *), void *child_stack), int flags, void *args)
      
    int (*func)(void *);
    char *child_stack, *args;
    int flags;

    // read the function pointer
    if(argptr(0, (char **)&func, 0) == -1) {
        return -1;
    }
    
    // read the child stack address
    if(argint(1, (int *)&child_stack) == -1) {
        return -1;
    }
    
    // read the flags 
    if(argint(2, &flags) == -1) {
        return -1;
    }
    
    // read the address of argument 
    if(argptr(3, &args, 0) == -1) {
        return -1;
    }
    
    // the arguments passed to clone must have address below size
    if(THREAD_LEADER(myproc())->sz < (uint)args) {
        return -1;
    }

    // clone system call supports stack address space which are one page alligned 
    if(THREAD_LEADER(myproc())->sz <= (uint)child_stack && 
       THREAD_LEADER(myproc())->sz < (uint)child_stack - PGSIZE) {
        return -1;
    }
    
    // call to actual kernel clone function 
    return clone(func, (void *)child_stack, flags, (void *)args);
}

// sys join copies the contents from user stack to the kernel memory 
// sys join calls join which waits for the given thread to be over
int sys_join(void) {

    int tid;
    // read the thread id to be joined 
    if(argint(0, &tid) == -1) {
        return -1;
    }
    return join(tid);
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// kills paritcular thread in the group
int 
sys_tkill(void)
{
  int tid;

  if(argint(0, &tid) < 0)
    return -1;
  return tkill(tid);
}

// kills complete thread group
// kills all thread expect group leader
int 
sys_tgkill(void)
{
  return tgkill();
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

// gets thread id of the process 
// if process calls gettid pid is returned (single threaded process)
// if thread calls gettid tid is returned  (multithreaded process)
int 
sys_gettid(void) 
{
  return myproc()->tid == -1 ? sys_getpid() : myproc()->tid;    
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  acquire(&(THREAD_LEADER(myproc())->tlock));

  addr = THREAD_LEADER(myproc())->sz;
  if(growproc(n) < 0) {
    release(&(THREAD_LEADER(myproc())->tlock));
    return -1;
  }

  release(&(THREAD_LEADER(myproc())->tlock));

  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// sys suspend syspends the current thread execution 
int
sys_tsuspend(void) 
{
  return tsuspend();
}

// sys resumes the execution of the thread with given tid
int 
sys_tresume(void) 
{
  int tid;
  if(argint(0, &tid) < 0)
    return -1;
  return tresume(tid);
}


